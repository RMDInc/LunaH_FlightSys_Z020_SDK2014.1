/*
 * WriteToLogFile.h
 *
 *  Created on: Jul 26, 2017
 *      Author: GStoddard
 */

#ifndef WRITETOLOGFILE_H_
#define WRITETOLOGFILE_H_

#include <stdio.h>
#include "xil_printf.h"
#include "ff.h"
#include "LNumDigits.h"

#define LOG_FILE_BUFF_SIZE	200

int filptr_clogFile;

int WriteToLogFile(char * cWriteToLogFileBuff, int iNumBytesToWrite);

#endif /* WRITETOLOGFILE_H_ */
