/*
 * read_data_in.c
 *
 *  Created on: Feb 14, 2017
 *      Author: GStoddard
 */

#include "read_data_in.h"

int ReadDataIn(float fEnergySlope, float fEnergyIntercept)
{
	int sw = 0;						// switch to stop and return to main menu  0= default.  1 = return
	u32 data;
	int dram_ceiling;
	int dram_addr, array_index, diff;
	double bl1, bl2, bl3, bl4, bl_avg;	// bl_sum;

	struct event_raw *data_array;
	data_array = (struct event_raw *)malloc(sizeof(double)*512);

	array_index = 0;
	dram_addr = 0;
	bl1 = 0.0;
	bl2 = 0.0;
	bl3 = 0.0;
	bl4 = 0.0;
	bl_avg = 0.0;
	diff = 0;

	dram_addr = 0xa000000;		// Read only Adj Average data from DRAM
	dram_ceiling = 0xA004000; 	//read out just adjacent average (0xA004000 - 0xa000000 = 16384)	//167788544
//	dram_ceiling = 0xA00C000;	//

	Xil_DCacheInvalidateRange(0x00000000, 65536);

	//identify and test loop //looking for a false event or 111111 with a valid event following it
	while(1)
	{
		data = Xil_In32(dram_addr);

		if(data == 111111)
		{
			//check for a second identifier
			if(Xil_In32(dram_addr + 4) == 111111)
				dram_addr += 4;

		}
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
			data_array[array_index].total_events = (double)Xil_In32(dram_addr + 8);
			data_array[array_index].event_num = (double)Xil_In32(dram_addr + 12);
			data_array[array_index].bl = (double)Xil_In32(dram_addr + 16) / ( 16.0 * 38.0 );

			bl4 = bl3; bl3 = bl2; bl2 = bl1;
			bl1 = data_array[array_index].bl;
			if(bl4 == 0.0)
				bl_avg = bl1;
			else
				bl_avg = (bl1 + bl2 + bl3 + bl4) / 4.0;

			data_array[array_index].si = ((double)Xil_In32(dram_addr + 20) / 16.0) - bl_avg * 73.0;
			data_array[array_index].li = ((double)Xil_In32(dram_addr + 24) / 16.0) - bl_avg * 169.0;
			data_array[array_index].fi = ((double)Xil_In32(dram_addr + 28) / 16.0) - bl_avg * 1551.0;
			data_array[array_index].psd = data_array[array_index].si / (data_array[array_index].li - data_array[array_index].si);
			data_array[array_index].energy = fEnergySlope * data_array[array_index].fi + fEnergyIntercept;
			++array_index;
			dram_addr = dram_addr + 32;		//align the loop with the data
		}
		else {
			dram_addr += 4;
		}
	}	// end of while loop

	//SD card stuff //uncomment when ready to test SD card
	/*
	static FIL file1;
	char filenameBin[20] = {};
	FRESULT res;				// Return variable for SD functions
	uint numBytesWritten;
	int fileSize = 0;

	Xil_DCacheFlush();
	Xil_DCacheDisable();

	//open, write to, and close the data file //writes one buffer at a time
	res = f_open(&file1, filenameBin, FA_OPEN_ALWAYS | FA_WRITE);	// Open the file if it exists, if not create a new file; file has write permission
	if(res)
		sw = 1;
	res = f_lseek(&file1, file1.fsize);						// Move the file write pointer to somewhere in the file
	res = f_write(&file1, (const void*)data_array, fileSize, &numBytesWritten);	// Write the array data from above to the file, returns the #bytes written
	res = f_close(&file1);		// Close the file on the SD card
	*/

	free(data_array);			// Return the space reserved for the array back to the OS
	return sw;
} //eof
