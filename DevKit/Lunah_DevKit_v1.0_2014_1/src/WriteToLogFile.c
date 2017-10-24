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
	returnVal = f_lseek(&logFile, filptr_clogFile);
	returnVal = f_write(&logFile, cWriteToLogFileBuff, iNumBytesToWrite, &numBytesWritten);
	filptr_clogFile += numBytesWritten;
	snprintf(cWriteToLogFileBuff, 10, "%d", filptr_clogFile);							// Write that to a string for saving
	returnVal = f_lseek(&logFile, (10 - LNumDigits(filptr_clogFile)));				// Seek to the beginning of the file skipping the leading zeroes
	returnVal = f_write(&logFile, cWriteToLogFileBuff, LNumDigits(filptr_clogFile), &numBytesWritten); // Write the new file pointer
	returnVal = f_close(&logFile);

	return success;
}
