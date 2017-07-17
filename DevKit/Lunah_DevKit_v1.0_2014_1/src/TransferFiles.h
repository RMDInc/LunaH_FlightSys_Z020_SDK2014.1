/*
 * TransferFiles.h
 *
 *  Created on: Jul 13, 2017
 *      Author: GStoddard
 */

#ifndef TRANSFERFILES_H_
#define TRANSFERFILES_H_

#include <stdio.h>		//needed for unsigned types
#include <stdlib.h>
#include <string.h>
#include <xil_io.h>
#include "ff.h"
#include "xuartps.h"	//needed for uart functions

int TransferFiles(char * fileName, XUartPs *Uart_PS);
#endif /* TRANSFERFILES_H_ */
