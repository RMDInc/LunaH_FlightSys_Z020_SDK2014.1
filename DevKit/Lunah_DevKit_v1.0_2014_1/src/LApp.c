/******************************************************************************
*
* Main Dev Kit Application
* Revision 1.0
*
* Notes:
* Initial Software package that is developed for the Dev Kit.  07/06/16 - EBJ
*
******************************************************************************/

#include "LApp.h"

//////////////////////////// MAIN //////////////////// MAIN //////////////
int main()
{
	//Variable Definitions

	int iterator = 0;
	int	imenusel = 99999;	// Menu Select
	int iwfRunNumber = 0;
	int idaqRunNumber = 0;
	int iorbitNumber = 0;
	char cCNTFileName[FILENAME_SIZE] = "";
	char cEVTFileName[FILENAME_SIZE] = "";
	char cWFDFileName[FILENAME_SIZE] = "";
	char cFileToAccess[FILENAME_SIZE] = "";
	char cReportBuff[100] = "";
	unsigned char errorBuff[] = "FFFFFF";
	long long int iRealTime = 0;
	unsigned int uiEndTime = 0;
	char cWriteBREAK[] = "BREAK requested ";
	char cBREAK[] = "FAFAFA";
	int iTriggerThreshold = 0;
	int ipollReturn = 0;
	int iintegrationTimes[4] = {};
	FRESULT fresult;
	FILINFO fnoDIR2;

	//test variables
	int sent = 0;
	int returnVal = 0;
	int bytesSent = 0;
	int buffsize = 0; 	//BRAM buffer size
	int counter = 0;

	//transfer file variables
	FIL fnoFileToTransfer;
	int iFileSize = 0;
	unsigned int iSprintfReturn = 0;
	uint numBytesRead = 0;
	int totalBytesRead = 0;

	//test write zeroes
	FIL zeroFile;
	FRESULT returnValue = 0;

	unsigned int numBytesWritten = 0;	//try unsigned int instead of uint
	unsigned int uiZeroData[] = {111111, 12345, 678, 12, 34, 56, 78, 90};

	//header
	unsigned int uiHeaderInfo = 0;
	unsigned int ucFileType = 0;
	//case 2
	int iTmpSetTemp = 0;
	int iTmpTimeout = 0;
	//case 3
	unsigned int uiTotalNeutronsPSD = 0;
	unsigned int uiLocalTime = 0;
	int iAnalogTemp = 12;
	int iDigitalTemp = 34;
	// case 6
	int index = 0;
	unsigned int sync_marker = 0x352EF853;
	unsigned char seq_count_holder = 0;
	unsigned int checksum1 = 0;
	unsigned int checksum2 = 0;
	unsigned char cTransferFileContents[DATA_PACKET_SIZE] = ""; //sizeof(data_packet) = 10+2028+2 = 2040 bytes
	//case 10
	float fNCut0 = 0.0;
	float fNCut1 = 0.0;
	float fNCut2 = 0.0;
	float fNCut3 = 0.0;
	//case 13
	float fEnergySlope = 1.0;
	float fEnergyIntercept = 0.0;

	// Initialize System
    init_platform();  		// This initializes the platform, which is ...
	ps7_post_config();
	Xil_DCacheDisable();	//
	InitializeAXIDma();		// Initialize the AXI DMA Transfer Interface
	Xil_Out32 (XPAR_AXI_GPIO_16_BASEADDR, 16384);
	Xil_Out32 (XPAR_AXI_GPIO_17_BASEADDR , 1);
	InitializeInterruptSystem(XPAR_PS7_SCUGIC_0_DEVICE_ID);

	//*******************Setup the UART **********************//
	XUartPs_Config *Config = XUartPs_LookupConfig(UART_DEVICEID);
	if (Config == NULL) { return 1;}
	Status = XUartPs_CfgInitialize(&Uart_PS, Config, Config->BaseAddress);
	if (Status != 0)
		xil_printf("UART init failed\r\nStatus = %d",Status);

	/* Set to normal mode. */
	XUartPs_SetOperMode(&Uart_PS, XUARTPS_OPER_MODE_NORMAL);
	//*******************Setup the UART **********************//

	// *********** Setup the Hardware Reset GPIO ****************//
	GPIOConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
	Status = XGpioPs_CfgInitialize(&Gpio, GPIOConfigPtr, GPIOConfigPtr ->BaseAddr);
	if (Status != XST_SUCCESS) { return XST_FAILURE; }
	XGpioPs_SetDirectionPin(&Gpio, SW_BREAK_GPIO, 1);	//break is 18
	// *********** Setup the Hardware Reset MIO ****************//

	// *********** Mount SD Card and Initialize Variables ****************//
	filptr_clogFile = 0;
	if( doMount == 0 ){			//Initialize the SD card here
		ffs_res = f_mount(0, NULL);
		ffs_res = f_mount(0, &fatfs);
		doMount = 1;
	}

	//******************* Set Defaults **********************//
	fresult = f_stat( cDefLogFile, &fno);	// check if the defaults file exists
	switch(fresult)
	{
	case FR_OK:	// If the file exists, read it
		ffs_res = f_open(&defLogFile, cDefLogFile, FA_READ|FA_WRITE);	//open with read/write access
		ffs_res = f_lseek(&defLogFile, 0);								//move to the beginning of file, we want to
		//read the file
		ffs_res = f_read(&defLogFile, &ui_def_1, 4, &numBytesRead);
		ffs_res = f_read(&defLogFile, &ui_def_2, 4, &numBytesRead);
		ffs_res = f_read(&defLogFile, &ui_def_3, 4, &numBytesRead);
		ffs_res = f_read(&defLogFile, &ui_def_4, 4, &numBytesRead);

		//sort out what values were read from the file
		iTriggerThreshold = ui_def_1 >> 16;
		iintegrationTimes[0] = (ui_def_1 >> 4) & 4095;
		iintegrationTimes[1] = ((ui_def_1 & 15) << 8) | (ui_def_2 >> 24);
		iintegrationTimes[2] = (ui_def_2 >> 12) & 4095;
		iintegrationTimes[3] = ui_def_2 & 4095;

		ffs_res = f_close(&defLogFile);
		break;
	case FR_NO_FILE: //	if the file does not exist, open/create it
		ffs_res = f_open(&defLogFile, cDefLogFile, FA_WRITE|FA_OPEN_ALWAYS);	//open a new file; this creates the log file
		ui_def_1 = 557056000;
		ui_def_2 = 587740649;
		ffs_res = f_write(&defLogFile, &ui_def_1, 4, &numBytesWritten);
		ffs_res = f_write(&defLogFile, &ui_def_1, 4, &numBytesWritten);
		//add defaults 3 and 4 for the energy calibration
		// and any further values to record

		iTriggerThreshold = 8500;	//write out what the basic defaults should be
		iintegrationTimes[0] = 0;	//this is in case the file gets deleted, we have values to revert to
		iintegrationTimes[1] = 35;
		iintegrationTimes[2] = 131;
		iintegrationTimes[3] = 1513;
		ffs_res = f_close(&defLogFile);									//close the file
		break;
	default:
		bytesSent = XUartPs_Send(&Uart_PS, errorBuff,20);	//send a string
	}

	Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR, iintegrationTimes[0]);	// baseline
	Xil_Out32(XPAR_AXI_GPIO_1_BASEADDR, iintegrationTimes[1]);	// short
	Xil_Out32(XPAR_AXI_GPIO_2_BASEADDR, iintegrationTimes[2]);	// long
	Xil_Out32(XPAR_AXI_GPIO_3_BASEADDR, iintegrationTimes[3]);	// full
//	Xil_Out32(XPAR_AXI_GPIO_6_BASEADDR, 0);		//disable data acquisition
//	Xil_Out32(XPAR_AXI_GPIO_7_BASEADDR, 0);		//disable 5 volt sensor head
//	Xil_Out32(XPAR_AXI_GPIO_13_BASEADDR, 1);	//adc reset
	Xil_Out32(XPAR_AXI_GPIO_10_BASEADDR, iTriggerThreshold);	// set the trigger threshold

	//******************* Set Defaults **********************//

	fresult = f_stat( cLogFile, &fno);	// check if the command log file exists
	switch(fresult)
	{
	case FR_OK:	// If the file exists, read it
		ffs_res = f_open(&logFile, cLogFile, FA_READ|FA_WRITE);	//open with read/write access
		ffs_res = f_lseek(&logFile, f_size(&logFile));							//move to the end of file
		iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "POWER RESET %f ", dTime);	//write that the system was power cycled
		ffs_res = f_write(&logFile, cWriteToLogFile, iSprintfReturn, &numBytesWritten);	//write to the file
		ffs_res = f_close(&logFile);
		break;
	case FR_NO_FILE: //	if no file exists, so open/create the file
		ffs_res = f_open(&logFile, cLogFile, FA_WRITE|FA_OPEN_ALWAYS);	//open a new file; this creates the log file
		iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "SYSTEM ON %f", dTime);	//write the time the system is first booted
		ffs_res = f_close(&logFile);									//close the file
		break;
	default:
		bytesSent = XUartPs_Send(&Uart_PS, errorBuff,20);	//send a string
	}

	fresult = f_stat(cDirectoryLogFile0, &fnoDIR);	//check if the directory file exists
	switch(fresult)
	{
	case FR_OK:	//it exists, move on
		break;
	case FR_NO_FILE:	//it doesn't exist, create the file and write the command log file name and it's own file name
		ffs_res = f_open(&directoryLogFile, cDirectoryLogFile0, FA_WRITE|FA_OPEN_ALWAYS);	//create the file
		ffs_res = f_write(&directoryLogFile, cLogFile, 11, &numBytesWritten);				//write the name of the log file because it was created above
		ffs_res = f_write(&directoryLogFile, cDirectoryLogFile0, 17, &numBytesWritten);		//write the name of the Directory log file
		ffs_res = f_close(&directoryLogFile);												//close the file
		break;
	default:
		bytesSent = XUartPs_Send(&Uart_PS, errorBuff, 20);
	}
	// *********** Mount SD Card and Initialize Variables ****************//

	//test area
	/* this method was successful for writing/reading floats to a binary SD card file */
//	FIL float_test_fil;
//	char c_float_test[] = "float_test.bin";
//	double d_test_num_1 = 123.456;
//	double d_test_num_2 = 0.01243;
//	double d_holder = 0.0;
//
//	//open a new sd card file, write in the numbers, try and read out the numbers
//	ffs_res = f_open(&float_test_fil, c_float_test, FA_WRITE|FA_READ|FA_OPEN_ALWAYS);	//create the file
//	ffs_res = f_write(&float_test_fil, &d_test_num_1, sizeof(double), &numBytesWritten);
//	ffs_res = f_write(&float_test_fil, &d_test_num_2, sizeof(double), &numBytesWritten);

//	ffs_res = f_lseek(&float_test_fil, 0);
//	ffs_res = f_read(&float_test_fil, &d_holder, sizeof(double), &numBytesRead);
//	printf("num_1 = %f\r\n", d_holder);
//	d_holder = 0.0;
//	ffs_res = f_read(&float_test_fil, &d_holder, sizeof(double), &numBytesRead);
//	printf("num_2 = %f\r\n", d_holder);
//
//	ffs_res = f_close(&float_test_fil);
//	ffs_res = f_unlink(c_float_test);
	//end test area

	// Initialize buffers
	memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
	memset(SendBuffer, '0', 32);	// Clear SendBuffer Variable

	// ******************* POLLING LOOP *******************//
	while(1){
		XUartPs_SetOptions(&Uart_PS,XUARTPS_OPTION_RESET_RX);	// Clear UART Read Buffer
		memset(RecvBuffer, '\0', 32);							// Clear RecvBuffer Variable
		iPollBufferIndex = 0;									// Reset the variable keeping track of entered characters in the receive buffer
		while(1)
		{
			imenusel = 99999;
			imenusel = ReadCommandType(RecvBuffer, &Uart_PS);

			//now use the return value to figure out what to do with this information
			if ( imenusel >= -2 && imenusel <= 17 )
				break;

			// This code replicates the per second data heartbeat
			sleep(1);
			iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d_%u_%u", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			uiTotalNeutronsPSD += 25;
			uiLocalTime += 1;
		}

		switch (imenusel) { // Switch-Case Menu Select
		case -2: //Test case statement for bringing DAQ back into the project
			//change parameters, prepare system to run
			Xil_Out32(XPAR_AXI_GPIO_14_BASEADDR, (u32)4);	//enable processed data, change mode
			usleep(1);
			Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, (u32)1);	//enables system
			usleep(1);
			iTriggerThreshold = 0;
			Xil_Out32(XPAR_AXI_GPIO_10_BASEADDR, (u32)iTriggerThreshold);	//set trigger threshold
			usleep(1);

			XUartPs_SetOptions(&Uart_PS,XUARTPS_OPTION_RESET_RX);
			iPollBufferIndex = 0;
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			while(1)
			{
				//check for user input
				ipollReturn = ReadCommandType(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14)
					break;

				//check the FPGA buffers
				buffsize = Xil_In32(XPAR_AXI_GPIO_11_BASEADDR);	//how full are the buffers?
				if(buffsize >= 4095)
				{
					DAQ();
					PrintData();
					printf("\r\n%d\r\n",counter++);
				}
				else
					printf("\r\nWaiting for buffer to fill...\r\n");

				//check if it's time to write SOH information
				//...
				//...
			}
			break;
		case -1:
			iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			break;
		case 0:	//Capture Processed Data;
			sscanf(RecvBuffer + 3 + 1, " %d", &iorbitNumber);

			//create files and pass in filename to readDataIn function
			snprintf(cEVTFileName, 50, "%04d_%04d_evt.bin",iorbitNumber, idaqRunNumber);	//assemble the filename for this DAQ run
			snprintf(cCNTFileName, 50, "%04d_%04d_cnt.bin",iorbitNumber, idaqRunNumber);	//assemble the filename for this DAQ run
			iSprintfReturn = snprintf(cReportBuff, 100, "%s_%s", cEVTFileName, cCNTFileName);	//create the string to tell CDH
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);				//echo the name to bus? //if they aren't tracking file names, then don't do this

			iPollBufferIndex = 0;			// Reset the variable keeping track of entered characters in the receive buffer
			while(1)
			{
				ipollReturn = ReadCommandType(RecvBuffer, &Uart_PS);	//begin polling for either 'break' or 'START'
				if(ipollReturn == 14 || ipollReturn == 15)	//14=break, 15=start_orbitnumber
					break;

				// This code replicates the per second data heart beat
				sleep(1);
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d_%u_%u", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
				uiTotalNeutronsPSD += 25;
				uiLocalTime += 1;
			}
			if(ipollReturn == 14)	//if the input was break, leave the loop, go back to menu
			{
				Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cBREAK, 6);	//write FAFAFA to indicate to the user that the "BREAK" was successful
				break;
			}

			//'START'(15) begins data collection via DAQ(), saves the file names, command input
			idaqRunNumber++;
			sscanf(RecvBuffer + 5 + 1, " %llu", &iRealTime);

			//enable hardware
			Xil_Out32(XPAR_AXI_GPIO_14_BASEADDR, (u32)4);	//enable processed data
			usleep(1);
			Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, (u32)1);	//enables system
			usleep(1);
//			Xil_Out32(XPAR_AXI_GPIO_6_BASEADDR, (u32)1);	//enables system
//			usleep(1);
//			Xil_Out32(XPAR_AXI_GPIO_7_BASEADDR, (u32)1);	//enables system
//			usleep(1);

			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "DAQ Run %s %s %llu ", cEVTFileName,cCNTFileName, iRealTime);	//write to log file that we are starting a DAQ run
			WriteToLogFile(cWriteToLogFile, iSprintfReturn);

			//create the header by bit shifting, math
			uiHeaderInfo = iorbitNumber;		//and onto the header the orbit number
			uiHeaderInfo = uiHeaderInfo << 16;	//shift it over
			uiHeaderInfo += idaqRunNumber;		//and on the run number

			//open the files for the run and write the headers
			returnValue = f_open(&zeroFile, cCNTFileName, FA_OPEN_ALWAYS | FA_WRITE);
			returnValue = f_lseek(&zeroFile, f_size(&zeroFile));
			returnValue = f_write(&zeroFile, &iRealTime, 8, &numBytesWritten);	//real time is 64 bits = 8 bytes
			returnValue = f_write(&zeroFile, &uiHeaderInfo, 4, &numBytesWritten);	//header info is 24 bit empty + 16 bit orbit num + 16 bit run num + 8 bit data type
			ucFileType = 0;
			returnValue = f_write(&zeroFile, &ucFileType, 1, &numBytesWritten);
			returnValue = f_close(&zeroFile);

			returnValue = f_open(&zeroFile, cEVTFileName, FA_OPEN_ALWAYS | FA_WRITE);
			returnValue = f_lseek(&zeroFile, f_size(&zeroFile));
			returnValue = f_write(&zeroFile, &iRealTime, 8, &numBytesWritten);
			returnValue = f_write(&zeroFile, &uiHeaderInfo, 4, &numBytesWritten);
			ucFileType = 2;
			returnValue = f_write(&zeroFile, &ucFileType, 1, &numBytesWritten);
			returnValue = f_close(&zeroFile);

			//Begin collecting data
			uiTotalNeutronsPSD = 0;
			uiLocalTime = 0;
			iPollBufferIndex = 0;			// Reset the variable keeping track of entered characters in the receive buffer
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			while(1)
			{
				ipollReturn = PollUart(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 16)	//14=break, 16=END_realtime
					break;
				if(ipollReturn == 18)	//tell the user that the input was bad and the buffer was reset
				{
					memset(RecvBuffer, '0', 32);
					iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
					bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
				}

				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d_%u_%u", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
				uiLocalTime += 1;

				if(Xil_In32(XPAR_AXI_GPIO_11_BASEADDR) > 4095)	//the value 4095 will change when we update to Meg's new bitstream
				{
					DAQ();
					uiTotalNeutronsPSD += ReadDataIn(cCNTFileName, cEVTFileName,1.0, 0.0);	//give the count filename, event filename, slope, y-intercept
				}

				//for "normal" operation, this will just loop constantly, polling the receive buffer and the gpio
				//also, the 'per second' data heartbeat won't be reported by this function, it will be handled by another thread
				//but since that isn't in place yet, just sleep for a second here
				sleep(1);
			}
			if(ipollReturn == 16)	//if the input was END, leave the loop, go back to menu
			{
				uiEndTime = iRealTime + uiLocalTime;

				returnValue = f_open(&zeroFile, cCNTFileName, FA_OPEN_ALWAYS | FA_WRITE);	//write the final realtime to the file as a footer
				returnValue = f_lseek(&zeroFile, f_size(&zeroFile));
				returnValue = f_write(&zeroFile, &uiEndTime, sizeof(uiEndTime), &numBytesWritten);
				returnValue = f_close(&zeroFile);

				returnValue = f_open(&zeroFile, cEVTFileName, FA_OPEN_ALWAYS | FA_WRITE);	//write the final realtime to the file as a footer
				returnValue = f_lseek(&zeroFile, f_size(&zeroFile));
				returnValue = f_write(&zeroFile, &uiEndTime, sizeof(uiEndTime), &numBytesWritten);
				returnValue = f_close(&zeroFile);

				iSprintfReturn = snprintf(cReportBuff, 100, "%u", uiTotalNeutronsPSD);	//return value for END_
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);

				//write to the log file that we received END_ and the number of neutrons
				iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "END DAQ Run %u %llu ", uiTotalNeutronsPSD, iRealTime);	//write to log file that we are starting a DAQ run
				WriteToLogFile(cWriteToLogFile, iSprintfReturn);
			}
			else	//break was issued
			{
				Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				WriteToLogFile(cWriteBREAK, sizeof(cWriteBREAK));
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cBREAK, 6);	//write FAFAFA to indicate to the user that the "BREAK" was successful
			}

			sw = 0;	//reset stop switch
			break;
		case 1:	//Capture WF Data
			sscanf(RecvBuffer + 2 + 1, " %d", &iorbitNumber);

			//turn on hardware
			sleep(1);

			//create files and pass in filename to readDataIn function
			snprintf(cWFDFileName, 50, "%04d_%04d_wfd.bin",iorbitNumber, iwfRunNumber);	//assemble the filename for this DAQ run

			iSprintfReturn = snprintf(cReportBuff, 100, "%s", cWFDFileName);		//create the string to tell CDH
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);		//report to CDH

			//begin polling for either 'break', 'end', or 'START'
			iPollBufferIndex = 0;			// Reset the variable keeping track of entered characters in the receive buffer
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			XUartPs_SetOptions(&Uart_PS,XUARTPS_OPTION_RESET_RX);	// Clear UART Read Buffer
			while(1)
			{
				ipollReturn = ReadCommandType(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 15)	//14=break, 15=START_realtime
					break;

				// This code replicates the per second data heartbeat
				sleep(1);
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d_%u_%u", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
				uiTotalNeutronsPSD += 25;
				uiLocalTime += 1;
			}
			if(ipollReturn == 14)	//if the input was break, leave the loop, go back to menu
			{
				Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cBREAK, 6);	//write FAFAFA to indicate to the user that the "BREAK" was successful
				break;
			}

			//'START' begins data collection via DAQ(), etc.
			iwfRunNumber++;
			sscanf(RecvBuffer + 5 + 1, " %llu", &iRealTime);

			//enable hardware
			Xil_Out32(XPAR_AXI_GPIO_14_BASEADDR, 0);	//enable AA waveform mode
			Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 1);	//enables system

			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "WF run %s %llu ", cWFDFileName, iRealTime);
			WriteToLogFile(cWriteToLogFile, iSprintfReturn);

			//Begin collecting data
			//DAQ(fEnergySlope, fEnergyIntercept);
			//just print out some random data while polling for break, end, etc
			uiTotalNeutronsPSD = 0;
			uiLocalTime = 0;
			iPollBufferIndex = 0;			// Reset the variable keeping track of entered characters in the receive buffer
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			while(1)
			{
				ipollReturn = PollUart(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 16)	//14=break, 16=END_realtime
					break;
				if(ipollReturn == 18)	//if the input was checked and reset, inform the user that it is bad
				{
					memset(RecvBuffer, '0', 32);
					iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
					bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
				}

				//xil_printf("\r\n%d_%d_%x_%x\r\n", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d_%u_%u", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);			//create the string to tell CDH
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
				uiTotalNeutronsPSD += 25;
				uiLocalTime += 1;

				//write zeroes to a file code
				returnValue = f_open(&zeroFile, cWFDFileName, FA_OPEN_ALWAYS | FA_WRITE);
				returnValue = f_lseek(&zeroFile, f_size(&zeroFile));
				returnValue = f_write(&zeroFile, uiZeroData, sizeof(uiZeroData), &numBytesWritten);
				returnValue = f_close(&zeroFile);

				sleep(2);
			}
			if(ipollReturn == 16)	//if the input was END, leave the loop, go back to menu
			{
				//write the realtime to the file as a footer

				//Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				iSprintfReturn = snprintf(cReportBuff, 100, "%u", uiTotalNeutronsPSD);			//create the string to tell CDH
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			else	//break was issued
			{
				Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				WriteToLogFile(cWriteBREAK, sizeof(cWriteBREAK));
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cBREAK, 6);	//write FAFAFA to indicate to the user that the "BREAK" was successful
			}

			sw = 0;	//reset stop switch
			break;
		case 2: //Set Temperature	//Just Read Temp for now...
			sscanf(RecvBuffer + 3 + 1, " %d_%d", &iTmpSetTemp, &iTmpTimeout);
			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "TMP set %d %d ", iTmpSetTemp, iTmpTimeout);
			WriteToLogFile(cWriteToLogFile, iSprintfReturn);

			/* Set temp loop */
			IIC_SLAVE_ADDR=&IIC_SLAVE_ADDR2;
			i2c_Send_Buffer[0] = 0x0;
			i2c_Send_Buffer[1] = 0x0;
			//Status = IicPsMasterSend(IIC_DEVICE_ID, i2c_Send_Buffer, i2c_Recv_Buffer, IIC_SLAVE_ADDR);
			//Status = IicPsMasterRecieve(IIC_DEVICE_ID, i2c_Recv_Buffer, IIC_SLAVE_ADDR);
			a = i2c_Recv_Buffer[0]<< 5;
			b = a | i2c_Recv_Buffer[1] >> 3;
			b /= 16;
			//xil_printf("%d\xf8\x43\n", b);
			iPollBufferIndex = 0;			// Reset the variable keeping track of entered characters in the receive buffer
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			for(iterator = 1; iterator < iTmpTimeout; iterator++)
			{
				ipollReturn = PollUart(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 17)	//14=break, 17=ENDTMP_
					break;
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d_%u_%u", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);			//create the string to tell CDH
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
				sleep(2);
			}
			if(ipollReturn == 14)	//if the input was break, leave the loop, go back to menu
			{
				WriteToLogFile(cWriteBREAK, sizeof(cWriteBREAK));
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cBREAK, 6);	//write FAFAFA to indicate to the user that the "BREAK" was successful
			}
			else if(ipollReturn == 17)	//ENDTMP was issued
			{
				iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "END TMP run %d %d ", iAnalogTemp, iterator);	//want this to read in the current temp and the time elapsed
				WriteToLogFile(cWriteToLogFile, iSprintfReturn);
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d", iAnalogTemp, iterator);	//send back what the current temp is plus how long we ran for
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);	//send a string
			}
			else	// timeout was reached
			{
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d", iAnalogTemp, iTmpTimeout);	//send back what the current temp is plus how long we ran for
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);	//send a string
				//just disable the system, no need to inform that things are ok
			}
			break;
		case 3: //GETSTAT
			//this will be the callback function for the interrupts that we will use to send the heartbeat information
			iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d_%u_%u", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);	//send a string
			break;
		case 4: //DISABLE_ACT
			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "DISABLE_ACT requested ");
			WriteToLogFile(cWriteToLogFile, iSprintfReturn);
			iSprintfReturn = snprintf(cReportBuff, 100, "AAAAAA");
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			break;
		case 5: //DISABLE_TEC
			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "DISABLE_TEC requested ");
			WriteToLogFile(cWriteToLogFile, iSprintfReturn);
			iSprintfReturn = snprintf(cReportBuff, 100, "AAAAAA");
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			break;
		case 6: //Transfer files
			sscanf(RecvBuffer + 2 + 1, " %s", cFileToAccess);	//read in the name of the file to be transmitted
			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "file TX %s ", cFileToAccess);
			WriteToLogFile(cWriteToLogFile, iSprintfReturn);

			sleep(1);	//test if the sd card needs a second to finish writing the file

			returnValue = f_open(&fnoFileToTransfer, cFileToAccess, FA_READ);	//open the file to transfer
			if(returnValue != FR_OK)
			{
				iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
				break;
			}

			sent = 0;
			returnVal = 0;

			//initialize the data packet with parameters that won't change
			memset(cTransferFileContents, 0, DATA_PACKET_SIZE);
			cTransferFileContents[0] = sync_marker >> 24;
			cTransferFileContents[1] = sync_marker >> 16;
			cTransferFileContents[2] = sync_marker >> 8;
			cTransferFileContents[3] = sync_marker >> 0;
			cTransferFileContents[4] = 43;
			cTransferFileContents[5] = 43;
			cTransferFileContents[6] = 43;
			cTransferFileContents[7] = 0;

			returnValue = f_lseek(&fnoFileToTransfer, 0);	//seek to the beginning of the file
			while(1)
			{
				returnValue = f_read(&fnoFileToTransfer, (void *)&(cTransferFileContents[10]), PAYLOAD_MAX_SIZE, &numBytesRead);	//read 100 bytes at a time until we are through with the file

				iFileSize = numBytesRead + 2 - 1;	//number of bytes from the read plus the two byte checksum minus one
				cTransferFileContents[8] = iFileSize >> 8;
				cTransferFileContents[9] = iFileSize;

				for(index = 0; index < numBytesRead; index++)
				{
					checksum1 = (checksum1 + cTransferFileContents[index+10]) % 255;				//simple checksum
					checksum2 = (checksum1 + checksum2) % 255;			//advanced checksum
				}

				//check if we had enough data to fill the payload buffer
				if(numBytesRead < PAYLOAD_MAX_SIZE)
				{
					//finish filling the buffer
					for(; index < PAYLOAD_MAX_SIZE; index++)
					{
						cTransferFileContents[index+10] = 0;
					}
				}

				cTransferFileContents[DATA_PACKET_SIZE-2] = checksum2;
				cTransferFileContents[DATA_PACKET_SIZE-1] = checksum1;

				//send the data out over the UART
				sent = 0;		//reset the number of bytes sent
				returnVal = 0;	//reset the number of bytes sent
				//set the remaining unset bits to 0xFF = 255;
//				printf("\r\n%c_%c_%c_%c_%c_%c_%c_%c_%c_%c\r\n",cTransferFileContents[0],cTransferFileContents[1],cTransferFileContents[2],cTransferFileContents[3],cTransferFileContents[4],cTransferFileContents[5],cTransferFileContents[6],cTransferFileContents[7],cTransferFileContents[8],cTransferFileContents[9]);
//				printf("%c_%c\r\n",cTransferFileContents[DATA_PACKET_SIZE-2],cTransferFileContents[DATA_PACKET_SIZE-1]);
//				xil_printf("num_bytes_read=%d\r\n",numBytesRead);
//
//				//try writing just the header to the UART at first, then sending the packet
//				//this will tell the function exactly how many bytes there are and hopefully it will accept nulls
				returnVal = XUartPs_Send(&Uart_PS,&(cTransferFileContents[0]), 10);	//send just the header

				while(1)
				{
					returnVal = XUartPs_Send(&Uart_PS, &(cTransferFileContents[10]) + sent, (DATA_PACKET_SIZE - 10) - sent);	//pass the buffer to the bus (RS 422?)
					sent += returnVal;			//we want to start farther into the buffer each round
					if(sent == (DATA_PACKET_SIZE - 10))	//if we have sent the same number of bytes as the size of the buffer, we are done
						break;
				}
//				xil_printf("\r\n%d\r\n",sent);

				//we want to break out once the file is totally read
				if(numBytesRead < PAYLOAD_MAX_SIZE)
					break;
				else
				{
					//if we aren't done reading yet, reset variables
//					memset(cTransferFileContents, 0, DATA_PACKET_SIZE);

					//determine if we need to roll over the sequence count value
					if(cTransferFileContents[7] < 255)	// (2^8)-1	//if we are not ready to roll over the small sequence count yet
						cTransferFileContents[7]++;
					else								// or, if we are, roll over the small sequence count, 255 -> 0
					{
						seq_count_holder = cTransferFileContents[6] << 2;	//get rid of the first two bits
						seq_count_holder = cTransferFileContents[6] >> 2;	//shift back to get the sequence count bits back in place
						cTransferFileContents[7] = 0;		//roll over the small count
						if(seq_count_holder < 63)		//if the big count is small enough
							cTransferFileContents[6] += 1;//add one to the total
						else
						{
							cTransferFileContents[6] & 192;	//mask with 1100 0000 to preserve the first two bits, but zero out the msb of sequence count
							break;
						}
					}
				}
			}//end of while

			returnValue = f_close(&fnoFileToTransfer);	//close the transfer file

			break;
		case 7: //Delete Files
			sscanf(RecvBuffer + 3 + 1, " %s", cFileToAccess);	//read in the name of the file to be deleted
			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "file DEL %s ", cFileToAccess);
			WriteToLogFile(cWriteToLogFile, iSprintfReturn);

			if(!f_stat(cFileToAccess, &fnoDIR2))	//returns 0 (false) if the file exists // !0 = true
			{
				ffs_res = f_unlink(cFileToAccess);
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_AAAA", ffs_res);
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			else
			{
				iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			break;
		case 8: //List Data Files
			//Just going to use the code from TX_filename
			sent = 0;
			returnVal = 0;
			totalBytesRead = 0;
			memset(cTransferFileContents, ' ', 101);	//reset the buffer

			returnValue = f_open(&fnoFileToTransfer, cDirectoryLogFile0, FA_READ);
			iFileSize = f_size(&fnoFileToTransfer);
			returnValue = f_lseek(&fnoFileToTransfer, 0);	//seek to the beginning of the file
			while(totalBytesRead < iFileSize)
			{
				returnValue = f_read(&fnoFileToTransfer, &(cTransferFileContents[0]), 100, &numBytesRead);	//read 100 bytes at a time until we are through with the file
				totalBytesRead += numBytesRead;
				iSprintfReturn = snprintf((char *)cTransferFileContents, numBytesRead, (char *)cTransferFileContents + '\0');

				sent = 0;
				returnVal = 0;
				while(1)
				{
					returnVal = XUartPs_Send(&Uart_PS, &(cTransferFileContents[0]) + sent, iSprintfReturn - sent);
					sent += returnVal;			//we want to start farther into the buffer each round
					if(sent == iSprintfReturn)	//if we have sent the same number of bytes as the size of the buffer, we are done
						break;
				}
			}

			returnValue = f_close(&fnoFileToTransfer);
			break;
		case 9: //Set Trigger Threshold
			sscanf(RecvBuffer + 3 + 1," %d", &iTriggerThreshold);	//read in value from the recvBuffer
			if((iTriggerThreshold > 0) && (iTriggerThreshold < 10000))				//check that it's within accepted values
			{
				Xil_Out32(XPAR_AXI_GPIO_10_BASEADDR, (u32)iTriggerThreshold);
				iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "Set trigger threshold to %d ", iTriggerThreshold);
				WriteToLogFile(cWriteToLogFile, iSprintfReturn);

				//read back value from the FPGA
				iTriggerThreshold = 0;	//clear the number before reading back to it
				iTriggerThreshold = Xil_In32(XPAR_AXI_GPIO_10_BASEADDR);
				iSprintfReturn = snprintf(cReportBuff, 100, "%d", iTriggerThreshold);
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			else
			{
				iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			break;
		case 10: //Set Neutron Cuts
			sscanf(RecvBuffer + 6 + 1, " %f_%f_%f_%f", &fNCut0, &fNCut1, &fNCut2, &fNCut3);
			if((fNCut0 < fNCut1) && (fNCut2 < fNCut3))
			{
				iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "set n cuts %f %f %f %f ", fNCut0, fNCut1, fNCut2, fNCut3);
				WriteToLogFile(cWriteToLogFile, iSprintfReturn);
				iSprintfReturn = snprintf(cReportBuff, 100, "%f_%f_%f_%f", fNCut0, fNCut1, fNCut2, fNCut3);
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			else
			{
				iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			break;
		case 11: //Set High Voltage	//only works for 4 HV connections for now (should only be 4 per board)
			IIC_SLAVE_ADDR=&IIC_SLAVE_ADDR1;
			cntrl = 0x1; //command 1 - write contents of serial register data to RDAC - see AD5144 datasheet pg 26

			sscanf(RecvBuffer + 2 + 1, " %d_%d", &rdac, &data_bits);
			if((rdac >= 1)&&(rdac <= 5) && (data_bits >=1) && (data_bits <= 256))
			{
				switch(rdac){
				case 1:
					pot_addr = 0x0;
					break;
				case 2:
					pot_addr = 0x1;
					break;
				case 3:
					pot_addr = 0x2;
					break;
				case 4:
					pot_addr = 0x3;
					break;
				case 5:
					pot_addr = 0x8;
					break;
				default:
					pot_addr = 0xF;	//error val? Want to make sure this doesn't set anything
					break;
				}

				a = cntrl<<4|pot_addr;
				i2c_Send_Buffer[0] = a;
				i2c_Send_Buffer[1] = data_bits;
				//Status = IicPsMasterSend(IIC_DEVICE_ID, i2c_Send_Buffer, i2c_Recv_Buffer, IIC_SLAVE_ADDR);
				iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "set HV %d %d ", rdac, data_bits);
				WriteToLogFile(cWriteToLogFile, iSprintfReturn);
				//echo input to user to confirm
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d", rdac, data_bits);
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			else
			{
				iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			break;
		case 12: //Set Integration Times
			sscanf(RecvBuffer + 3 + 1, " %d_%d_%d_%d", &iintegrationTimes[0], &iintegrationTimes[1], &iintegrationTimes[2], &iintegrationTimes[3]);

			if((iintegrationTimes[0] < iintegrationTimes[1]) && ( iintegrationTimes[1] < iintegrationTimes[2]) && (iintegrationTimes[2] < iintegrationTimes[3]))	//if each is greater than the last
			{
				Xil_Out32 (XPAR_AXI_GPIO_0_BASEADDR, (u32)((iintegrationTimes[0]+52)/4));	//compute and change the values
				Xil_Out32 (XPAR_AXI_GPIO_1_BASEADDR, (u32)((iintegrationTimes[1]+52)/4));
				Xil_Out32 (XPAR_AXI_GPIO_2_BASEADDR, (u32)((iintegrationTimes[2]+52)/4));
				Xil_Out32 (XPAR_AXI_GPIO_3_BASEADDR, (u32)((iintegrationTimes[3]+52)/4));

				iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "Set integration times to %d %d %d %d ", (iintegrationTimes[0]+52)/4, (iintegrationTimes[1]+52)/4, (iintegrationTimes[2]+52)/4, (iintegrationTimes[3]+52)/4);
				WriteToLogFile(cWriteToLogFile, iSprintfReturn);

				//xil_printf("\r\n%d_%d_%d_%d\r\n", iintegrationTimes[0], iintegrationTimes[1], iintegrationTimes[2], iintegrationTimes[3]);
				iSprintfReturn = snprintf(cReportBuff, 100, "%d_%d_%d_%d",iintegrationTimes[0], iintegrationTimes[1], iintegrationTimes[2], iintegrationTimes[3]);
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			else
			{
				iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
				bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			}
			break;
		case 13: //Set Calibration Parameters	//these will modify the energy calculation in read_data_in
			sscanf(RecvBuffer + 4 + 1, " %f_%f", &fEnergySlope, &fEnergyIntercept);
			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "energy calib %f %f ", fEnergySlope, fEnergyIntercept);
			WriteToLogFile(cWriteToLogFile, iSprintfReturn);
			iSprintfReturn = snprintf(cReportBuff, 100, "%f_%f",fEnergySlope, fEnergyIntercept);
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			break;
		default :
			iSprintfReturn = snprintf(cReportBuff, 100, "FFFFFF");
			bytesSent = XUartPs_Send(&Uart_PS, (u8 *)cReportBuff, iSprintfReturn);
			break;
		} // End Switch-Case Menu Select

	}	// ******************* POLLING LOOP *******************//

    cleanup_platform();
    return 0;
}
//////////////////////////// MAIN //////////////////// MAIN //////////////

//////////////////////////// InitializeAXIDma////////////////////////////////
// Sets up the AXI DMA
int InitializeAXIDma(void) {
	u32 tmpVal_0 = 0;

	tmpVal_0 = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x30);

	tmpVal_0 = tmpVal_0 | 0x1001; //<allow DMA to produce interrupts> 0 0 <run/stop>

	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x30, tmpVal_0);
	Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x30);

	return 0;
}
//////////////////////////// InitializeAXIDma////////////////////////////////

//////////////////////////// InitializeInterruptSystem////////////////////////////////
int InitializeInterruptSystem(u16 deviceID) {
	int Status;
	static XScuGic_Config *GicConfig; 	// GicConfig
	XScuGic InterruptController;		// Interrupt controller

	GicConfig = XScuGic_LookupConfig (deviceID);

	if(NULL == GicConfig) {

		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig, GicConfig->CpuBaseAddress);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;

	}

	Status = SetUpInterruptSystem(&InterruptController);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;

	}

	Status = XScuGic_Connect (&InterruptController,
			XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR,
			(Xil_ExceptionHandler) InterruptHandler, NULL);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;

	}

	XScuGic_Enable(&InterruptController, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR );

	return XST_SUCCESS;

}
//////////////////////////// InitializeInterruptSystem////////////////////////////////


//////////////////////////// Interrupt Handler////////////////////////////////
void InterruptHandler (void ) {

	u32 tmpValue;
	tmpValue = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x34);
	tmpValue = tmpValue | 0x1000;
	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x34, tmpValue);
}
//////////////////////////// Interrupt Handler////////////////////////////////


//////////////////////////// SetUp Interrupt System////////////////////////////////
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr) {
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, XScuGicInstancePtr);
	Xil_ExceptionEnable();
	return XST_SUCCESS;

}
//////////////////////////// SetUp Interrupt System////////////////////////////////

//////////////////////////// ReadCommandPoll////////////////////////////////
// Function used to clear the read buffer
// Read In new command, expecting a <return>
// Returns buffer size read
int ReadCommandPoll() {
	u32 rbuff = 0;			// read buffer size returned
	int i = 0; 				// index
	XUartPs_SetOptions(&Uart_PS,XUARTPS_OPTION_RESET_RX);	// Clear UART Read Buffer
	for (i=0; i<32; i++ ) { RecvBuffer[i] = '_'; }			// Clear RecvBuffer Variable
	while (!(RecvBuffer[rbuff-1] == '\n' || RecvBuffer[rbuff-1] == '\r')) {
		rbuff += XUartPs_Recv(&Uart_PS, (u8 *)&RecvBuffer[rbuff],(32 - rbuff));
		sleep(0.1);			// Built in Latency ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 0.1 s
	}
	return rbuff;

}
//////////////////////////// ReadCommandPoll////////////////////////////////

//////////////////////////// PrintData ////////////////////////////////
int PrintData( ){
	unsigned int data;
	int dram_addr;
	int dram_base = 0xa000000;
	int dram_ceiling = 0xA004000; //read out just adjacent average (0xA004000 - 0xa000000 = 16384)
//	int dram_ceiling = 0xA00C000; //read out all three buffers     (0xA00C000 - 0xa000000 = 65536)	//can't do this with the bitstream as of 10/10/17

	Xil_DCacheInvalidateRange(0xa0000000, 65536);

	for (dram_addr = dram_base; dram_addr <= dram_ceiling; dram_addr+=4)	//just for testing, leave printf in
	{
		data = Xil_In32(dram_addr);
		xil_printf("%d\r\n",data);
	}

	return sw;
}
//////////////////////////// PrintData ////////////////////////////////

//////////////////////////// DAQ ////////////////////////////////
int DAQ(){
	Xil_Out32 (XPAR_AXI_GPIO_15_BASEADDR, 1);
	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x48, 0xa000000);
	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x58 , 65536);
	usleep(54);

	Xil_Out32 (XPAR_AXI_GPIO_15_BASEADDR, 0);

	Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,1);	//Clear buffers, sleep 1 us
	usleep(10);
	Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,0);

	return sw;
}
//////////////////////////// DAQ ////////////////////////////////

//////////////////////////// getWFDAQ ////////////////////////////////

int getWFDAQ(){
	int buffsize; 	//BRAM buffer size
	//int dram_addr;	// DRAM Address

	XUartPs_SetOptions(&Uart_PS,XUARTPS_OPTION_RESET_RX);

	//Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x48, 0xa000000); 		// DMA Transfer Step 1
	//Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x58 , 65536);			// DMA Transfer Step 2
	//sleep(1);						// Built in Latency ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 1 s
	Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,1);
	Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,0);
	// Capture garbage in DRAM
	//for (dram_addr = 0xa000000; dram_addr <= 0xA004000; dram_addr+=4){Xil_In32(dram_addr);}

	while(1)
	{
//		if (!sw) { sw = XGpioPs_ReadPin(&Gpio, SW_BREAK_GPIO); } //read pin
		XUartPs_Recv(&Uart_PS, (u8 *)&RecvBuffer, 32);
		if ( RecvBuffer[0] == 'q' ) { sw = 1; }
		if(sw) { return sw;	}

		buffsize = Xil_In32 (XPAR_AXI_GPIO_11_BASEADDR);	// AA write pointer // checks how many ints have been written
		if(buffsize >= 4095){
			Xil_Out32 (XPAR_AXI_GPIO_15_BASEADDR, 1);
			Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x48, 0xa000000);
			Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x58 , 65536);
			sleep(1); 			// Built in Latency ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 1 s
			Xil_Out32 (XPAR_AXI_GPIO_15_BASEADDR, 0);

			PrintData();

			Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,1);
			Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,0);
		}
	}
	return sw;
}

//////////////////////////// getWFDAQ ////////////////////////////////
