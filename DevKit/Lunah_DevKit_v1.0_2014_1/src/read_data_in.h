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
#define	EVENTS_PER_BUFFER	512

///// Structure Definitions ////

struct event_raw {			// Structure is 8+4+8+8+8+8= 44 bytes long
	double time;
	long long total_events;
	long long event_num;
	double bl;
	double si;
	double li;
	double fi;
	double psd;
	double energy;
};

struct cps_data {
	unsigned short n_psd;
	unsigned short counts;
	unsigned short n_no_psd;
	unsigned short n_wide_cut;
	unsigned int time;
	unsigned char temp;
};

struct event_by_event {
	u16 u_EplusPSD;
	unsigned int ui_localTime;
	unsigned int ui_nEvents_temp_ID;
};

struct counts_per_second {
	unsigned int ui_nPSD_CNTsOverThreshold;
	unsigned int ui_nNoPSD_nWideCuts;
	unsigned int ui_localTime;
	u8 uTemp;
};

///// Function Definitions /////
int ReadDataIn(char * cCountFileName, char * cEventFileName, float fEnergySlope, float fEnergyIntercept);	// Print Data to the Terminal Window

#endif /* READ_DATA_IN_H_ */
