/*
 * ReadCommandType.h
 *
 *  Created on: Jul 11, 2017
 *      Author: GStoddard
 */

#ifndef READCOMMANDTYPE_H_
#define READCOMMANDTYPE_H_

#include <stdio.h>		//needed for unsigned types
#include "xuartps.h"	//needed for uart functions

int iPollBufferIndex;

int ReadCommandType(char * RecvBuffer, XUartPs *Uart_PS);
int PollUart(char * RecvBuffer, XUartPs *Uart_PS);

#endif /* READCOMMANDTYPE_H_ */
