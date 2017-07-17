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
	int iscanfReturn = 0;
	int iTransferReturn = 0;
	//unsigned long long realTime = 0;
	int iwfRunNumber = 0;
	int idaqRunNumber = 0;
	int iorbitNumber = 0;
	char cfileName[FILENAME_SIZE] = "";
	char cFileToAccess[FILENAME_SIZE] = "";
	int iTriggerThreshold = 0;
	int ipollReturn = 0;
	int iintegrationTimes[3] = {};
	FILINFO fnoDIR2;

	//case 2
	int iTmpSetTemp = 0;
	int iTmpTimeout = 0;
	//case 3
	//unsigned char cAnalogTemp = 12;
	//unsigned char cDigitalTemp = 34;
	u32 uiTotalNeutronsPSD = 294967295;
	u32 uiLocalTime = 1234567890;
	int iAnalogTemp = 12;
	int iDigitalTemp = 34;
	//case 10
	float fNCut0 = 0.0;
	float fNCut1 = 0.0;
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

	// Initialize buffers
	memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
	memset(SendBuffer, '0', 32);	// Clear SendBuffer Variable

	//*******************Setup the UART **********************//
	XUartPs_Config *Config = XUartPs_LookupConfig(UART_DEVICEID);
	if (NULL == Config) { return 1;}
	Status = XUartPs_CfgInitialize(&Uart_PS, Config, Config->BaseAddress);
	if (Status != 0){
		//return 1;	}
		xil_printf("UART init failed\n");
	}

	/* Conduct a Selftest for the UART */
	Status = XUartPs_SelfTest(&Uart_PS);
	if (Status != 0) {
		//return 1; }			//handle error checks here better
	xil_printf("UART self-test failed\n");
	}

	/* Set to normal mode. */
	XUartPs_SetOperMode(&Uart_PS, XUARTPS_OPER_MODE_NORMAL);
	//*******************Setup the UART **********************//

	//*******************Receive and Process Packets **********************//
	Xil_Out32 (XPAR_AXI_GPIO_0_BASEADDR, 11);	// setting defaults for integration times	//bl
	Xil_Out32 (XPAR_AXI_GPIO_1_BASEADDR, 71);	// short
	Xil_Out32 (XPAR_AXI_GPIO_2_BASEADDR, 167);	// long
	Xil_Out32 (XPAR_AXI_GPIO_3_BASEADDR, 2015);	// full
/*	Xil_Out32 (XPAR_AXI_GPIO_4_BASEADDR, 12);
	Xil_Out32 (XPAR_AXI_GPIO_5_BASEADDR, 75);
	Xil_Out32 (XPAR_AXI_GPIO_6_BASEADDR, 75);
	Xil_Out32 (XPAR_AXI_GPIO_7_BASEADDR, 5);
	Xil_Out32 (XPAR_AXI_GPIO_8_BASEADDR, 25);*/
	//*******************enable the system **********************//
	Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 1);
	//*******************Receive and Process Packets **********************//

	// *********** Setup the Hardware Reset GPIO ****************//
	GPIOConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
	Status = XGpioPs_CfgInitialize(&Gpio, GPIOConfigPtr, GPIOConfigPtr ->BaseAddr);
	if (Status != XST_SUCCESS) { return XST_FAILURE; }
	XGpioPs_SetDirectionPin(&Gpio, SW_BREAK_GPIO, 1);
	// *********** Setup the Hardware Reset MIO ****************//

	// *********** Mount SD Card and Initialize Variables ****************//
	if( doMount == 0 ){			//Initialize the SD card here
		ffs_res = f_mount(0, NULL);
		ffs_res = f_mount(0, &fatfs);
		doMount = 1;
	}
	if( f_stat( cLogFile, &fno) ){	// f_stat returns non-zero(false) if no file exists, so open/create the file
		ffs_res = f_open(&logFile, cLogFile, FA_WRITE|FA_OPEN_ALWAYS);
		ffs_res = f_write(&logFile, cZeroBuffer, 10, &numBytesWritten);
		filptr_clogFile += numBytesWritten;		// Protect the first xx number of bytes to use as flags, in this case xx must be 10
		ffs_res = f_close(&logFile);
	}
	else // If the file exists, read it
	{
		ffs_res = f_open(&logFile, cLogFile, FA_READ|FA_WRITE);	//open with read/write access
		ffs_res = f_lseek(&logFile, 0);							//go to beginning of file
		ffs_res = f_read(&logFile, &filptr_buffer, 10, &numBytesRead);	//Read the first 10 bytes to determine flags and the size of the write pointer
		sscanf(filptr_buffer, "%d", &filptr_clogFile);			//take the write pointer from char -> integer so we may use it
		ffs_res = f_lseek(&logFile, filptr_clogFile);			//move the write pointer so we don't overwrite info
		iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "POWER RESET %f ", dTime);	//write that the system was power cycled
		ffs_res = f_write(&logFile, cWriteToLogFile, iSprintfReturn, &numBytesWritten);	//write to the file
		filptr_clogFile += numBytesWritten;						//update the write pointer
		ffs_res = f_close(&logFile);							//close the file
	}

	if( f_stat(cDirectoryLogFile0, &fnoDIR) )	//check if the file exists
	{
		ffs_res = f_open(&directoryLogFile, cDirectoryLogFile0, FA_WRITE|FA_OPEN_ALWAYS);	//if no, create the file
		ffs_res = f_write(&directoryLogFile, cZeroBuffer, 10, &numBytesWritten);			//write the zero buffer so we can keep track of the write pointer
		filptr_cDIRFile += 10;																//move the write pointer
		ffs_res = f_write(&directoryLogFile, cLogFile, 11, &numBytesWritten);				//write the name of the log file because it was created above
		filptr_cDIRFile += numBytesWritten;													//update the write pointer
		snprintf(cWriteToLogFile, 10, "%d", filptr_cDIRFile);								//write formatted output to a sized buffer; create a string of a certain length
		ffs_res = f_lseek(&directoryLogFile, (10 - LNumDigits(filptr_cDIRFile)));			// Move to the start of the file
		ffs_res = f_write(&directoryLogFile, cWriteToLogFile, LNumDigits(filptr_cDIRFile), &numBytesWritten);	//Record the new file pointer
		ffs_res = f_close(&directoryLogFile);												//close the file
	}
	else	//if the file exists, read it
	{
		ffs_res = f_open(&directoryLogFile, cDirectoryLogFile0, FA_READ);					//open the file
		ffs_res = f_lseek(&directoryLogFile, 0);											//move to the beginning of the file
		ffs_res = f_read(&directoryLogFile, &filptr_cDIRFile_buffer, 10, &numBytesWritten);	//read the write pointer
		sscanf(filptr_cDIRFile_buffer, "%d", &filptr_cDIRFile);								//write the pointer to the relevant variable
		ffs_res = f_close(&directoryLogFile);												//close the file
	}
	// *********** Mount SD Card and Initialize Variables ****************//

	// ******************* POLLING LOOP *******************//
	while(1){
		XUartPs_SetOptions(&Uart_PS,XUARTPS_OPTION_RESET_RX);	// Clear UART Read Buffer
		memset(RecvBuffer, '0', 32);							// Clear RecvBuffer Variable

		while (XUartPs_IsSending(&Uart_PS)) { }  // Wait until Write Buffer is Sent

		while(1)
		{
			imenusel = 99999;
			memset(RecvBuffer, '\0', 32);							// Clear RecvBuffer Variable
			imenusel = ReadCommandType(RecvBuffer, &Uart_PS);

			//now use the return value to figure out what to do with this information1
			if ( imenusel >= 0 && imenusel <= 17 )
				break;
		}

		switch (imenusel) { // Switch-Case Menu Select
		case 0:	//Capture Processed Data;
			xil_printf("DAQ initiated\n\r");
			iscanfReturn = sscanf(RecvBuffer + 3 + 1, " %d", &iorbitNumber);

			//create files and pass in filename to readDataIn function
			snprintf(cfileName, 50, "%d_%d_evt.bin",iorbitNumber, idaqRunNumber);	//assemble the filename for this DAQ run
			snprintf(cfileName, 50, "%d_%d_cnt.bin",iorbitNumber, idaqRunNumber);	//assemble the filename for this DAQ run

			//return value for typing in DAQ_time is a filename
			xil_printf("\r\n%s\r\n",cfileName);

			//begin polling for either 'break', 'end', or 'START'
			while(1)
			{
				ipollReturn = ReadCommandType(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 15)	//14=break, 15=start_orbitnumber
					break;
			}
			if(ipollReturn == 14)	//if the input was break, leave the loop, go back to menu
			{
				Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				break;
			}

			//'START' begins data collection via DAQ(), etc.
			idaqRunNumber++;
			//enable hardware
			Xil_Out32(XPAR_AXI_GPIO_14_BASEADDR, 4);	//enable processed data
			Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 1);	//enables system???
			//write to log file that we are starting a DAQ run
			dTime += 1;
			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "Print proc data %f ", dTime);

			ffs_res = f_open(&logFile, cLogFile, FA_OPEN_ALWAYS | FA_WRITE);
			if(ffs_res){
				xil_printf("Could not open file %d", ffs_res);
				break;
			}
			ffs_res = f_lseek(&logFile, filptr_clogFile);
			ffs_res = f_write(&logFile, cWriteToLogFile, iSprintfReturn, &numBytesWritten);
			filptr_clogFile += numBytesWritten;
			snprintf(cWriteToLogFile, 10, "%d", filptr_clogFile);							// Write that to a string for saving
			ffs_res = f_lseek(&logFile, (10 - LNumDigits(filptr_clogFile)));				// Seek to the beginning of the file skipping the leading zeroes
			ffs_res = f_write(&logFile, cWriteToLogFile, LNumDigits(filptr_clogFile), &numBytesWritten); // Write the new file pointer
			ffs_res = f_close(&logFile);

			//Begin collecting data
			//DAQ(fEnergySlope, fEnergyIntercept);
			uiTotalNeutronsPSD = 0;
			uiLocalTime = 0;
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			while(1)
			{
				ipollReturn = PollUart(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 16)	//14=break, 16=END_realtime
					break;
				xil_printf("\r\n%d_%d_%x_%x\r\n", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
				uiTotalNeutronsPSD += 25;
				uiLocalTime += 1;
				sleep(2);
			}
			if(ipollReturn == 16)	//if the input was END, leave the loop, go back to menu
			{
				//write the realtime to the file as a footer

				//Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				xil_printf("\r\n%x\r\n",uiTotalNeutronsPSD);	//return value for END_time is the neutron totals
			}
			else	//break was issued
			{
				Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
			}

			sw = 0;	//reset stop switch
			break;
		case 1:	//Capture WF Data
			xil_printf("WF capture initiated\n\r");
			iscanfReturn = sscanf(RecvBuffer + 2 + 1, " %d", &iorbitNumber);

			//turn on hardware
			sleep(1);

			//create files and pass in filename to readDataIn function
			snprintf(cfileName, 50, "%d_%d_wfd.bin",iorbitNumber, iwfRunNumber);	//assemble the filename for this DAQ run

			//send filename to indicate "ready to go"
			xil_printf("\r\n%s\r\n",cfileName);

			//begin polling for either 'break', 'end', or 'START'
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			XUartPs_SetOptions(&Uart_PS,XUARTPS_OPTION_RESET_RX);	// Clear UART Read Buffer
			while(1)
			{
				ipollReturn = ReadCommandType(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 15)	//14=break, 15=START_realtime
					break;
			}
			if(ipollReturn == 14)	//if the input was break, leave the loop, go back to menu
			{
				Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				break;
			}

			//'START' begins data collection via DAQ(), etc.
			iwfRunNumber++;
			//enable hardware
			Xil_Out32(XPAR_AXI_GPIO_14_BASEADDR, 0);	//enable processed data
			Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 1);	//enables system???
			//write to log file that we are starting a DAQ run
			dTime += 1;
			iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "Print wf data %f ", dTime);

			ffs_res = f_open(&logFile, cLogFile, FA_OPEN_ALWAYS | FA_WRITE);
			if(ffs_res){
				//xil_printf("Could not open file %d", ffs_res);
				break;
			}
			ffs_res = f_lseek(&logFile, filptr_clogFile);
			ffs_res = f_write(&logFile, cWriteToLogFile, iSprintfReturn, &numBytesWritten);
			filptr_clogFile += numBytesWritten;
			snprintf(cWriteToLogFile, 10, "%d", filptr_clogFile);							// Write that to a string for saving
			ffs_res = f_lseek(&logFile, (10 - LNumDigits(filptr_clogFile)));				// Seek to the beginning of the file skipping the leading zeroes
			ffs_res = f_write(&logFile, cWriteToLogFile, LNumDigits(filptr_clogFile), &numBytesWritten); // Write the new file pointer
			ffs_res = f_close(&logFile);

			//Begin collecting data
			//DAQ(fEnergySlope, fEnergyIntercept);
			//just print out some random data while polling for break, end, etc
			uiTotalNeutronsPSD = 0;
			uiLocalTime = 0;
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			while(1)
			{
				ipollReturn = PollUart(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 16)	//14=break, 16=END_realtime
					break;
				xil_printf("\r\n%d_%d_%x_%x\r\n", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
				uiTotalNeutronsPSD += 25;
				uiLocalTime += 1;
				sleep(2);
			}
			if(ipollReturn == 16)	//if the input was break, leave the loop, go back to menu
			{
				//write the realtime to the file as a footer

				//Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
				xil_printf("\r\n%x\r\n",uiTotalNeutronsPSD);	//return value for END_time is the neutron totals
			}
			else	//break was issued
			{
				Xil_Out32(XPAR_AXI_GPIO_18_BASEADDR, 0);	//disable system
			}

			sw = 0;	//reset stop switch
			break;
		case 2: //Set Temperature	//Just Read Temp for now...
			iscanfReturn = sscanf(RecvBuffer + 3 + 1, " %d_%d", &iTmpSetTemp, &iTmpTimeout);
			IIC_SLAVE_ADDR=&IIC_SLAVE_ADDR2;
			i2c_Send_Buffer[0] = 0x0;
			i2c_Send_Buffer[1] = 0x0;
			//Status = IicPsMasterSend(IIC_DEVICE_ID, i2c_Send_Buffer, i2c_Recv_Buffer, IIC_SLAVE_ADDR);
			//Status = IicPsMasterRecieve(IIC_DEVICE_ID, i2c_Recv_Buffer, IIC_SLAVE_ADDR);
			a = i2c_Recv_Buffer[0]<< 5;
			b = a | i2c_Recv_Buffer[1] >> 3;
			b /= 16;
			//xil_printf("%d\xf8\x43\n", b);
			memset(RecvBuffer, '0', 32);	// Clear RecvBuffer Variable
			for(iterator = 0;iterator < iTmpTimeout; iterator++)
			{
				ipollReturn = PollUart(RecvBuffer, &Uart_PS);
				if(ipollReturn == 14 || ipollReturn == 17)	//14=break, 17=ENDTMP_
					break;
				xil_printf("\r\n%d_%d_%x_%x\r\n", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
				sleep(2);
			}
			break;
		case 3: //GETSTAT
			//this will be the callback function for the interrupts that we will use to send the heartbeat information
			//need to print out char-char-int-int (analog temp, digital temp, agg neutron w/psd, local time)
			xil_printf("\r\n%d_%d_%x_%x\r\n", iAnalogTemp, iDigitalTemp, uiTotalNeutronsPSD, uiLocalTime);
			break;
		case 4: //DISABLE_ACT
			xil_printf("\r\nAAAAAA\r\n");
			break;
		case 5: //DISABLE_TEC
			xil_printf("\r\nAAAAAA\r\n");
			break;
		case 6: //Transfer files
			//read in the name of the file to be transmitted
			iscanfReturn = sscanf(RecvBuffer + 2 + 1, " %s", cFileToAccess);
			iTransferReturn = TransferFiles(cFileToAccess, &Uart_PS);
			if(iTransferReturn != 0)
				xil_printf("\r\nFFFFFF\r\n");
			break;
		case 7: //Delete Files
			//read in the name of the file to be deleted
			iscanfReturn = sscanf(RecvBuffer + 3 + 1, " %s", cFileToAccess);
			if(!f_stat(cFileToAccess, &fnoDIR2))	//returns 0 (false) if the file exists // !0 = true
			{
				ffs_res = f_unlink(cFileToAccess);
				xil_printf("%\r\nd_AAAA\r\n",ffs_res);
			}
			else
				xil_printf("\r\nFFFFFF\r\n");
			break;
		case 8: //List Data Files
			ffs_res = f_open(&directoryLogFile, cDirectoryLogFile0, FA_READ);	//open the directory log file
			dirSize = f_size(&directoryLogFile) - 10;							//we don't want the first 10 bits
			filptr_cDIRFile = 10;												//set the read pointer after those bits
			dirFileContents = (char *)malloc(1 * dirSize + 1);							//reserve space for the file to be read to	//this is one char for each byte in the file
			ffs_res = f_lseek(&directoryLogFile, 10);							//move to the beginning of the file
			ffs_res = f_read(&directoryLogFile, dirFileContents, dirSize, &numBytesRead);	//read the file contents to dirFileContents
			ffs_res = f_close(&directoryLogFile);								//close the log file
			snprintf(dirFileContents, dirSize + 1, dirFileContents + '\0');		//append a null terminator
			xil_printf(dirFileContents);										//print out the contents (will be a transfer at a later time)
			free(dirFileContents);												//free the memory reserved by malloc
			break;
		case 9: //Set Trigger Threshold
			//read in value from the recvBuffer
			iscanfReturn = sscanf(RecvBuffer + 3 + 1," %d", &iTriggerThreshold);
			//check that it's within accepted values
			if((iTriggerThreshold > 0) && (iTriggerThreshold < 10000))
			{
				Xil_Out32(XPAR_AXI_GPIO_10_BASEADDR, (u32)iTriggerThreshold);

				//record the change in the log file
				dTime += 1;
				iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "Set trigger threshold to %d %f ", iTriggerThreshold, dTime);

				ffs_res = f_open(&logFile, cLogFile, FA_OPEN_ALWAYS | FA_WRITE);
				if(ffs_res){
					xil_printf("Could not open file %d\r\n", ffs_res);
					break;
				}
				ffs_res = f_lseek(&logFile, filptr_clogFile);
				ffs_res = f_write(&logFile, cWriteToLogFile, iSprintfReturn, &numBytesWritten);
				filptr_clogFile += numBytesWritten;
				snprintf(cWriteToLogFile, 10, "%d", filptr_clogFile);							// Write that to a string for saving
				ffs_res = f_lseek(&logFile, (10 - LNumDigits(filptr_clogFile)));				// Seek to the beginning of the file skipping the leading zeroes
				ffs_res = f_write(&logFile, cWriteToLogFile, LNumDigits(filptr_clogFile), &numBytesWritten); // Write the new file pointer
				ffs_res = f_close(&logFile);

				//read back value from the FPGA
				iTriggerThreshold = 0;	//clear the number before reading back to it
				iTriggerThreshold = Xil_In32(XPAR_AXI_GPIO_10_BASEADDR);
				xil_printf("\r\n%d\r\n",iTriggerThreshold);
			}
			else
				xil_printf("FFFFFF\r\n");
			break;
		case 10: //Set Neutron Cuts
			iscanfReturn = sscanf(RecvBuffer + 6 + 1, " %f_%f", &fNCut0, &fNCut1);
			xil_printf("\r\n%d_%d\r\n", (int)fNCut0, (int)fNCut1);
			break;
		case 11: //Set High Voltage	//only works for 4 HV connections for now (should only be 4 per board)
			IIC_SLAVE_ADDR=&IIC_SLAVE_ADDR1;
			cntrl = 0x1; //command 1 - write contents of serial register data to RDAC - see AD5144 datasheet pg 26

			iscanfReturn = sscanf(RecvBuffer + 2 + 1, " %d_%d", &rdac, &data_bits);
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

				//Write to the log file what was changed

				//echo input to user to confirm
				xil_printf("\r\n%d_%d\r\n",rdac,data_bits);
			}
			else
				xil_printf("FFFFFF\n\r");
			break;
		case 12: //Set Integration Times
			iscanfReturn = sscanf(RecvBuffer + 3 + 1, " %d_%d_%d_%d", &iintegrationTimes[0], &iintegrationTimes[1], &iintegrationTimes[2], &iintegrationTimes[3]);

			if((iintegrationTimes[0] < iintegrationTimes[1]) && ( iintegrationTimes[1] < iintegrationTimes[2]) && (iintegrationTimes[2] < iintegrationTimes[3]))	//if each is greater than the last
			{
				Xil_Out32 (XPAR_AXI_GPIO_0_BASEADDR, (u32)((iintegrationTimes[0]+52)/4));	//compute and change the values
				Xil_Out32 (XPAR_AXI_GPIO_1_BASEADDR, (u32)((iintegrationTimes[1]+52)/4));
				Xil_Out32 (XPAR_AXI_GPIO_2_BASEADDR, (u32)((iintegrationTimes[2]+52)/4));
				Xil_Out32 (XPAR_AXI_GPIO_3_BASEADDR, (u32)((iintegrationTimes[3]+52)/4));

				dTime += 1;
				iSprintfReturn = snprintf(cWriteToLogFile, LOG_FILE_BUFF_SIZE, "Set integration times to %d %d %d %d %f ", (iintegrationTimes[0]+52)/4, (iintegrationTimes[0]+52)/4, (iintegrationTimes[0]+52)/4, (iintegrationTimes[0]+52)/4, dTime);
				ffs_res = f_open(&logFile, cLogFile, FA_OPEN_ALWAYS | FA_WRITE);
				if(ffs_res){
					xil_printf("Could not open file %d\r\n", ffs_res);
					break;
				}
				ffs_res = f_lseek(&logFile, filptr_clogFile);
				ffs_res = f_write(&logFile, cWriteToLogFile, iSprintfReturn, &numBytesWritten);
				filptr_clogFile += numBytesWritten;
				snprintf(cWriteToLogFile, 10, "%d", filptr_clogFile);							// Write that to a string for saving
				ffs_res = f_lseek(&logFile, (10 - LNumDigits(filptr_clogFile)));				// Seek to the beginning of the file skipping the leading zeroes
				ffs_res = f_write(&logFile, cWriteToLogFile, LNumDigits(filptr_clogFile), &numBytesWritten); // Write the new file pointer
				ffs_res = f_close(&logFile);

				xil_printf("\r\n%d_%d_%d_%d\r\n", iintegrationTimes[0], iintegrationTimes[1], iintegrationTimes[2], iintegrationTimes[3]);
			}
			else
				xil_printf("\r\nFFFFFF\r\n");
			break;
		case 13: //Set Calibration Parameters
			//these will modify the energy calculation in read_data_in
			iscanfReturn = sscanf(RecvBuffer + 4 + 1, " %f_%f", &fEnergySlope, &fEnergyIntercept);
			xil_printf("\r\n%d_%d\r\n",(int)fEnergySlope, (int)fEnergyIntercept);
			break;
		default :
			xil_printf("FFFFFF\r\n");
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
	u32 tmpVal_1 = 0;

	tmpVal_0 = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x30);

	tmpVal_0 = tmpVal_0 | 0x1001; //<allow DMA to produce interrupts> 0 0 <run/stop>

	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x30, tmpVal_0);
	tmpVal_1 = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 0x30);	//what does the return value give us? What do we do with it?

	return 0;
}
//////////////////////////// InitializeAXIDma////////////////////////////////

//////////////////////////// InitializeInterruptSystem////////////////////////////////
int InitializeInterruptSystem(u16 deviceID) {
	int Status;

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

	global_frame_counter++;
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
		rbuff += XUartPs_Recv(&Uart_PS, &RecvBuffer[rbuff],(32 - rbuff));
		sleep(0.1);			// Built in Latency ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 0.1 s
	}
	return rbuff;

}
//////////////////////////// ReadCommandPoll////////////////////////////////

//////////////////////////// PrintData ////////////////////////////////
int PrintData( ){
	u32 data;
	int dram_addr;

	// Read only Adj Average data from DRAM
	int dram_base = 0xa000000;
	int dram_cieling = 0xA004000; //read out just adjacent average (0xA004000 - 0xa000000 = 16384)

	Xil_DCacheInvalidateRange(0x00000000, 65536);

	for (dram_addr = dram_base; dram_addr <= dram_cieling; dram_addr+=4){
		if (!sw) { sw = XGpioPs_ReadPin(&Gpio, SW_BREAK_GPIO); } //read pin
		data = Xil_In32(dram_addr);
		xil_printf("%d\r\n",data);
		// check the uart buffer for a 'q'
		XUartPs_Recv(&Uart_PS, &RecvBuffer, 32);
		if ( RecvBuffer[0] == 'q' ) { sw = 1;  }
		if(sw) { return sw; }
	}

	return sw;
}
//////////////////////////// PrintData ////////////////////////////////


//////////////////////////// Clear Processed Data Buffers ////////////////////////////////
void ClearBuffers() {
	Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,1);
	sleep(1);						// Built in Latency ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 1 s
	Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,0);
}
//////////////////////////// Clear Processed Data Buffers ////////////////////////////////

//////////////////////////// DAQ ////////////////////////////////
int DAQ(float fEnergySlope, float fEnergyIntercept){
	int pollReturn = 0;
	int buffsize = 0; 	//BRAM buffer size
	//int dram_addr = 0;	// DRAM Address
	static int filesWritten = 0;

	XUartPs_SetOptions(&Uart_PS,XUARTPS_OPTION_RESET_RX);

	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x48, 0xa000000); 		// DMA Transfer Step 1
	Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x58 , 65536);			// DMA Transfer Step 2
	sleep(1);						// Built in Latency ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 1 s
	ClearBuffers();												// Clear Buffers.
	// Capture garbage in DRAM
	//for (dram_addr = 0xa000000; dram_addr <= 0xA004000; dram_addr+=4){Xil_In32(dram_addr);}

	while(1){
		pollReturn = ReadCommandType(RecvBuffer, &Uart_PS);	//check each time through for a 'break'
		if(pollReturn==14){return 1;}						//received 'break'

		buffsize = Xil_In32 (XPAR_AXI_GPIO_11_BASEADDR);	//how full are the buffers?
		if(buffsize >= 4095){
			Xil_Out32 (XPAR_AXI_GPIO_15_BASEADDR, 1);
			Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x48, 0xa000000);
			Xil_Out32 (XPAR_AXI_DMA_0_BASEADDR + 0x58 , 65536);
			sleep(1); 			// Built in Latency ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 1 s
			Xil_Out32 (XPAR_AXI_GPIO_15_BASEADDR, 0);
			Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,1);
			Xil_Out32(XPAR_AXI_GPIO_9_BASEADDR,0);
			sw = ReadDataIn(fEnergySlope, fEnergyIntercept, filesWritten, &directoryLogFile);
			++filesWritten;
		}
	}

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
		if (!sw) { sw = XGpioPs_ReadPin(&Gpio, SW_BREAK_GPIO); } //read pin
		XUartPs_Recv(&Uart_PS, &RecvBuffer, 32);
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
