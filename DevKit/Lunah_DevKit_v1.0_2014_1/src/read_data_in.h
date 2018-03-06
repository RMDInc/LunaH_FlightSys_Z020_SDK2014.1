/*
 * read_data_in.h
 *
 *  Created on: Feb 14, 2017
 *      Author: GStoddard
 */

#ifndef READ_DATA_IN_H_
#define READ_DATA_IN_H_

///// Include statements that PrintData() needs
#include <stdio.h>
#include <stdlib.h>
#include "xgpiops.h"
#include "xuartps.h"

///// Include statements that SD card needs
#include "ff.h"
#include "xil_cache.h"
#include "LNumDigits.h"

///// Global Variables /////
#define SW_BREAK_GPIO		51
#define data_array_size		512
#define FILENAME_BUFF_SIZE	120
#define	EVENTS_PER_BUFFER	512

///// Function Definitions /////
int ReadDataIn(char * cCountFileName, char * cEventFileName, double d_EnergySlope, double d_EnergyIntercept);	// Print Data to the Terminal Window

#endif /* READ_DATA_IN_H_ */
