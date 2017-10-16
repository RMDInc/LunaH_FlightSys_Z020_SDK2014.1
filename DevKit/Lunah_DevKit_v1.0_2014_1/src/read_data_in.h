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
//#include <xil_io.h>
#include <stdlib.h>
//#include "xaxidma.h"
//#include "xtime_l.h"
#include "xgpiops.h"
#include "xuartps.h"

///// Include statements that SD card needs
//#include "xparameters.h"	/* SDK generated parameters */
//#include "xsdps.h"			/* SD device driver */
#include "ff.h"
#include "xil_cache.h"
#include "LNumDigits.h"

///// Global Variables /////
#define SW_BREAK_GPIO		51
#define data_array_size		512
#define FILENAME_BUFF_SIZE	120

///// Structure Definitions ////

struct event_raw {			// Structure is 8+4+8+8+8+8= 44 bytes long
	double time;
	double total_events;
	double event_num;
	double bl;
	double si;
	double li;
	double fi;
	double psd;
	double energy;
};

///// Function Definitions /////
int ReadDataIn(float fEnergySlope, float fEnergyIntercept);	// Print Data to the Terminal Window

#endif /* READ_DATA_IN_H_ */
