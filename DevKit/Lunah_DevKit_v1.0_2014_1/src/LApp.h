/*
 * LApp.h
 *
 *  Created on: Jul 6, 2016
 *      Author: EJohnson
 */

#ifndef LAPP_H_
#define LAPP_H_

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "ps7_init.h"
#include <xil_io.h>
#include <xil_exception.h>
#include "xscugic.h"
#include "xaxidma.h"
#include "xparameters.h"
#include "platform_config.h"
#include "xgpiops.h"
#include "xuartps.h"
#include "xil_printf.h"
#include "sleep.h"

///SD Card Includes
#include "xparameters.h"	// SDK generated parameters
#include "xsdps.h"			// SD device driver
#include "ff.h"
#include "xil_cache.h"

// IiC Interface
//#include "LI2C_Interface.h"
#include "read_data_in.h"
#include "ReadCommandType.h"
#include "TransferFiles.h"
#include "WriteToLogFile.h"

/* Globals */
#define LOG_FILE_BUFF_SIZE	200
#define UART_DEVICEID		XPAR_XUARTPS_0_DEVICE_ID
#define SW_BREAK_GPIO		51
#define IIC_DEVICE_ID		XPAR_XIICPS_0_DEVICE_ID
#define TEST_BUFFER_SIZE	2
#define FILENAME_SIZE		50
#define DATA_PACKET_SIZE	2040
#define PAYLOAD_MAX_SIZE	2028

// The variable keeping track of how many characters have been entered into the recv buffer
int iPollBufferIndex;

// Hardware Interface
XUartPs Uart_PS;					// Instance of the UART Device
XUartPsFormat Uart_Format;			// Specifies the format we have
XGpioPs Gpio;						// Instance of the GPIO Driver Interface
XGpioPs_Config *GPIOConfigPtr;		// GPIO configuration pointer

/* FAT File System Variables */
FATFS fatfs;
FRESULT ffs_res;
FILINFO fno;
FILINFO fnoDIR;
int doMount = 0;
char cZeroBuffer[] = "0000000000 ";
char cLogFile[] = "LogFile.txt";	//Create a log file and file pointer
char c_default_log_file[] = "default_log_file.bin";
FIL logFile;
FIL default_log_file;
char filptr_buffer[11] = {};		// Holds 10 numbers and a null terminator
int filptr_clogFile;
char cDirectoryLogFile0[] = "DirectoryFile.txt";	//Directory File to hold all filenames
FIL directoryLogFile;
char filptr_cDIRFile_buffer[11] = {};
int filptr_cDIRFile = 0;

char cWriteToLogFile[LOG_FILE_BUFF_SIZE] = "";			//The buffer for adding information to the log file
int iSprintfReturn = 0;
double dTime = 12345.67;
uint numBytesWritten = 0;
uint numBytesRead = 0;

int dirSize = 0;
char * dirFileContents;

/* UART Variables */
static u8 SendBuffer[32];		// Buffer for Transmitting Data	// Used for RecvCommandPoll()
static char RecvBuffer[32];		// Buffer for Receiving Data

/* Defaults */
unsigned int ui_def_1 = 0;
unsigned int ui_def_2 = 0;
unsigned int ui_def_3 = 0;
unsigned int ui_def_4 = 0;

/* Check the size of the BRAM Buffer */
u32 databuff = 0;		// size of the data buffer

/* I2C Variables */
u8 i2c_Send_Buffer[TEST_BUFFER_SIZE];
u8 i2c_Recv_Buffer[2];
int IIC_SLAVE_ADDR1 = 0x20;
int IIC_SLAVE_ADDR2 = 0x48;
int *IIC_SLAVE_ADDR;		//pointer to slave
int rdac = 0;
int data_bits = 0;
int a, b;					//used for bitwise operations
short cntrl = 0;
short pot_addr = 0;

// General Purpose Variables
int retVal = 0;	// A return value
int Status;		// General purpose Status indicator
int sw = 0;		// switch to stop and return to main menu  0= default.  1 = return
int i = 0;		// Iterator in some places
unsigned char system_mode = 0;	//overall system mode	//0x0 = standby, main menu //0x1 = DAQ //0x2 = WF //0x4 = set TMP //0x8 = TX file

// Methods
int InitializeAXIDma(void); 		// Initialize AXI DMA Transfer
int InitializeInterruptSystem(u16 deviceID);
void InterruptHandler (void );
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr);
int ReadCommandPoll();				// Read Command Poll Function
void SetIntegrationTimes();			// Set the Registers forIntegral Times
int PrintData();					// Print Data to the Terminal Window
void ClearBuffers();				// Clear Processed Data Buffers
int DAQ();							// Clear Processed Data Buffers
int getWFDAQ();						// Print data skipping saving it to SD card
int LNumDigits(int number);			// Determine the number of digits in an integer

#endif /* LAPP_H_ */
