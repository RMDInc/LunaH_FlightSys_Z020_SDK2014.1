/*
 * WriteToLogFile.c
 *
 *  Created on: Jul 26, 2017
 *      Author: GStoddard
 */

#include "WriteToLogFile.h"

int WriteToLogFile(char * cWriteToLogFileBuff, int iNumBytesToWrite)
{
	//variables
	int success = 1;	//true = 1
	FIL logFile;
	FRESULT returnVal = 0;
	unsigned int numBytesWritten = 0;	//try unsigned int instead of uint
	char cLogFile[] = "LogFile.txt";	//Create a log file and file pointer

	/* Open the Log File */
	returnVal = f_open(&logFile, cLogFile, FA_OPEN_ALWAYS | FA_WRITE);
	if(returnVal)
		success = 0;

	/* Seek to the correct place in the file */
	returnVal = f_lseek(&logFile, f_size(&logFile));
	returnVal = f_write(&logFile, cWriteToLogFileBuff, iNumBytesToWrite, &numBytesWritten);
	returnVal = f_close(&logFile);

	return success;
}
