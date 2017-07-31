/*
 * TransferFiles.c
 *
 *  Created on: Jul 13, 2017
 *      Author: GStoddard
 */
#include "TransferFiles.h"

FIL fnoFileToTransfer;
FILINFO fnoFileInfo;

int TransferFiles(char * fileName,  XUartPs *InstancePtr) {
	int success = 0;
	int iFileSize = 0;
	int sent = 0;
	int returnVal = 0;
	int iSprintfReturn = 0;
	uint numBytesRead = 0;
	int totalBytesRead = 0;
	//unsigned char * cTransferFileContents;
	unsigned char cTransferFileContents[101] = "";
	FRESULT result;

	XUartPs_SetOptions(&InstancePtr,XUARTPS_OPTION_RESET_TX);	// Clear UART Transmit Buffer
	if(!f_stat(fileName, &fnoFileInfo))	//returns 0 (false) if the file exists // !0 = true
	{
		result = f_open(&fnoFileToTransfer, fileName, FA_READ);
		if(result != FR_OK)
			success = 1;
		iFileSize = f_size(&fnoFileToTransfer);
		//cTransferFileContents = (unsigned char *)calloc( iFileSize + 1, sizeof(unsigned char));
		//memset(cTransferFileContents, 0, iFileSize+1);

		result = f_lseek(&fnoFileToTransfer, 0);	//seek to the beginning of the file
		while(totalBytesRead < iFileSize)
		{
			result = f_read(&fnoFileToTransfer, &(cTransferFileContents[0]), 100, &numBytesRead);	//read 100 bytes at a time until we are through with the file
			totalBytesRead += numBytesRead;
			result = f_close(&fnoFileToTransfer);
			iSprintfReturn = snprintf(cTransferFileContents, numBytesRead, cTransferFileContents + '\0');

			sent = 0;
			returnVal = 0;
			while(1)
			{
				returnVal = XUartPs_Send(&InstancePtr, &(cTransferFileContents[0]) + sent, iSprintfReturn - sent);
				sent += returnVal;			//we want to start farther into the buffer each round
				if(sent == iSprintfReturn)	//if we have sent the same number of bytes as the size of the buffer, we are done
					break;
			}
		}
	}
	else
		success = 1;
	return success;
}
