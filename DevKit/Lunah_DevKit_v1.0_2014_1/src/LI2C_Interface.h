/*
 * LI2C_Interface.h
 *
 *  Created on: Mar 16, 2017
 *      Author: GStoddard
 */

#ifndef LI2C_INTERFACE_H_
#define LI2C_INTERFACE_H_

#include "xparameters.h"
#include "ps7_init.h"
#include "platform_config.h"
#include "xiicps.h"
#include "xil_printf.h"
#include "unistd.h"
#include "xuartps.h"

#include <xil_io.h>
#include <stdio.h>

#define IIC_SCLK_RATE		90000
#define TEST_BUFFER_SIZE	2

/* Function Declarations */
int IicPsMasterSend(u16 DeviceId, u8 * ptr_Send_Buffer, u8 * ptr_Recv_Buffer, int * iI2C_slave_addr);
int IicPsMasterRecieve(u16 DeviceId, u8 * ptr_Recv_Buffer, int * iI2C_slave_addr);

#endif /* LI2C_INTERFACE_H_ */
