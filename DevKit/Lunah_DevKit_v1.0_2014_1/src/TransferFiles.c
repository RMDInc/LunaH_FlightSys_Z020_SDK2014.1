/*
 * TransferFiles.c
 *
 *  Created on: Jul 13, 2017
 *      Author: GStoddard
 */
#include "TransferFiles.h"

FIL fnoFileToTransfer;

int TransferFiles(char * fileName,  XUartPs *Uart_PS) {
	int success = 0;
	int iFileSize = 0;
	uint numBytesRead = 0;
	char * cTransferFileContents;
	FRESULT result;

	if(!f_stat(fileName, &fnoFileToTransfer))	//returns 0 (false) if the file exists // !0 = true
	{
		result = f_open(&fnoFileToTransfer, fileName, FA_READ);
		iFileSize = f_size(&fnoFileToTransfer);
		cTransferFileContents = (char *)malloc(1 * iFileSize + 1);
		memset(cTransferFileContents, 0, iFileSize+1);
		result = f_lseek(&fnoFileToTransfer, 0);
		result = f_read(&fnoFileToTransfer, cTransferFileContents, iFileSize, &numBytesRead);
		result = f_close(&fnoFileToTransfer);
		snprintf(cTransferFileContents, iFileSize + 1, cTransferFileContents + '\0');
		//write some code to transfer the large buffer using the UART



		xil_printf("\r\n");
		xil_printf(cTransferFileContents);
		xil_printf("\r\n");
		free(cTransferFileContents);
	}
	else
		success = 1;
	return success;
}
