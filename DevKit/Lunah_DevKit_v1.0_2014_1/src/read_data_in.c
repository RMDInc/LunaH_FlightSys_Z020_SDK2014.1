/*
 * read_data_in.c
 *
 *  Created on: Feb 14, 2017
 *      Author: GStoddard
 */

#include "read_data_in.h"

int ReadDataIn(char * cCountFileName, char * cEventFileName, float fEnergySlope, float fEnergyIntercept)
{
	int sw = 0;						// switch to stop and return to main menu  0= default.  1 = return
	unsigned int n_psd_total = 0;
	unsigned int data = 0;
	int dram_ceiling = 0;
	int dram_addr = 0;
	int array_index = 0;
	int diff = 0;
	double bl1, bl2, bl3, bl4, bl_avg;	// bl_sum;

	double e_cut_primary_low = 80000.0;		//these are the neutron cut values
	double e_cut_primary_high = 200000.0;	//energy along the x-axis
	double psd_cut_primary_low = 0.5;		//psd along the y-axis
	double psd_cut_primary_high = 1.2;

	double e_cut_secondary_low = e_cut_primary_low * 0.75;	//these are the neutron wide cut values
	double e_cut_secondary_high = e_cut_primary_high * 1.25;
	double psd_cut_secondary_low = psd_cut_primary_low * 0.75;
	double psd_cut_secondary_high = psd_cut_primary_high * 1.25;

	struct event_raw *data_array;
	data_array = (struct event_raw *)malloc(sizeof(double)*EVENTS_PER_BUFFER);

	struct cps_data *mns_cps;
	mns_cps = (struct cps_data *)malloc(sizeof(struct cps_data)*EVENTS_PER_BUFFER);

	static FIL file1;
	FRESULT res;				// Return variable for SD functions
	uint numBytesWritten;
	int bytes_to_write = 0;

	bl1 = 0.0;
	bl2 = 0.0;
	bl3 = 0.0;
	bl4 = 0.0;
	bl_avg = 0.0;

	dram_addr = 0xa000000;		// Read only Adj Average data from DRAM
	dram_ceiling = 0xA004000; 	//read out just adjacent average (0xA004000 - 0xa000000 = 16384)	//167788544
//	dram_ceiling = 0xA00C000;	//this will work with the newer bitstreams

	Xil_DCacheInvalidateRange(0xa0000000, 65536);	//invalidate the range of addresses starting with dram_addr (the base value)

	//identify the first good value and skip the rest	//is this valid? we should probably just record all the data	//this will depend on if/how we are processing the data
	while(1)
	{
		data = Xil_In32(dram_addr);
		if(data == 111111)
			break;
		else
			dram_addr += 4;
	}

	//sort and process loop
	while (dram_addr <= dram_ceiling) //only read in if we are within the dram addresses that have data
	{
		//read in the data
		data = Xil_In32(dram_addr);
		diff = (dram_ceiling - dram_addr) / 4;	//looks for the end of the buffer so we may stop processing if we get too close

		//sort the data into the struct from above //also process the data
		if( data == 111111 && diff > 7 )
		{
			data_array[array_index].time = (double)Xil_In32(dram_addr + 4) * 262.144e-6;
			data_array[array_index].total_events = Xil_In32(dram_addr + 8);
			data_array[array_index].event_num = Xil_In32(dram_addr + 12);
			data_array[array_index].bl = (double)Xil_In32(dram_addr + 16) / ( 16.0 * 38.0 );

			bl4 = bl3; bl3 = bl2; bl2 = bl1;	//baseline avg sorting scheme
			bl1 = data_array[array_index].bl;
			if(bl4 == 0.0)						//if we haven't collected 4 baselines yet
				bl_avg = bl1;					// use the current value
			else								//else compute the running average
				bl_avg = (bl1 + bl2 + bl3 + bl4) / 4.0;

			data_array[array_index].si = ((double)Xil_In32(dram_addr + 20) / 16.0) - bl_avg * 73.0;
			data_array[array_index].li = ((double)Xil_In32(dram_addr + 24) / 16.0) - bl_avg * 169.0;
			data_array[array_index].fi = ((double)Xil_In32(dram_addr + 28) / 16.0) - bl_avg * 1551.0;
			data_array[array_index].psd = data_array[array_index].si / (data_array[array_index].li - data_array[array_index].si);
			data_array[array_index].energy = fEnergySlope * data_array[array_index].fi + fEnergyIntercept;

			//check the primary neutron cuts
			if((psd_cut_primary_low < data_array[array_index].psd) && (data_array[array_index].psd < psd_cut_primary_high))
				if((e_cut_primary_low < data_array[array_index].energy) && (data_array[array_index].energy < e_cut_primary_high))
					mns_cps[array_index].n_psd++;

			//check the energy cuts only (n w/no psd)
			if((e_cut_primary_low < data_array[array_index].energy) && (data_array[array_index].energy < e_cut_primary_high))
				mns_cps[array_index].n_no_psd++;

			//check the secondary neutron cuts (n wide cut)
			if((psd_cut_secondary_low < data_array[array_index].psd) && (data_array[array_index].psd < psd_cut_secondary_high))
				if((e_cut_secondary_low < data_array[array_index].energy) && (data_array[array_index].energy < e_cut_secondary_high))
					mns_cps[array_index].n_wide_cut++;

			//finish filling the values for the cps event
			mns_cps[array_index].counts = data_array[array_index].total_events;
			mns_cps[array_index].time = data_array[array_index].time;
			mns_cps[array_index].temp = 25;

			++array_index;
			dram_addr = dram_addr + 32;		//align the loop with the data
		}
		else
			dram_addr += 4;
	}	// end of while loop

	//write the event-by-event file
	res = f_open(&file1, cEventFileName, FA_OPEN_ALWAYS | FA_WRITE);	// Open the file if it exists, if not create a new file; file has write permission
	if(res)
		sw = 1;
	res = f_lseek(&file1, file1.fsize);		// Move the file write pointer to the end of the file
	bytes_to_write = sizeof(struct event_raw) * EVENTS_PER_BUFFER;
	res = f_write(&file1, (const void*)data_array, bytes_to_write, &numBytesWritten);	// Write the array data from above to the file, returns the # bytes written
	res = f_close(&file1);					// Close the file on the SD card

	//write the cps file
	res = f_open(&file1, cCountFileName, FA_OPEN_ALWAYS | FA_WRITE);	// Open the file if it exists, if not create a new file; file has write permission
	if(res)
		sw = 1;
	res = f_lseek(&file1, file1.fsize);		// Move the file write pointer to the end of the file
	bytes_to_write = sizeof(struct cps_data) * EVENTS_PER_BUFFER;
	res = f_write(&file1, (const void*)mns_cps, bytes_to_write, &numBytesWritten);	// Write the array data from above to the file, returns the # bytes written
	res = f_close(&file1);					// Close the file on the SD card

	n_psd_total = mns_cps[array_index-1].n_psd;

	free(data_array);			// Return the space reserved for the array back to the OS
	free(mns_cps);
	return n_psd_total;
} //eof
