#ifndef _CONTROL_H
#define _CONTROL_H

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"


struct SaveSector
{
	int32_t gyrocenter[3];
	int32_t acccenter[3]; //Third is unused.
	int32_t	accelmin[3]; //First 2 are unused.
	int32_t	accelmax[3]; //First 2 are unused.
	int32_t	magmin[3];
	int32_t	magmax[3];

	int32_t PIDVals[12]; //P, I, D, MAX (For all 3 PIDs (SPIN, LEFTRIGHT, FWDBAK)
};

extern struct SaveSector settings;


void ControlInit();
void SaveSettings();
void StartRange();
void DoZero();
void ResetIIR();
void ICACHE_FLASH_ATTR controltimer();

void ICACHE_FLASH_ATTR issue_command(void *arg, char *pusrdata, unsigned short len);

int ICACHE_FLASH_ATTR FillRaw( char * buffer );

extern struct espconn *pespconn;

#define BARIIRDEP 5

#define GYRIIRDEP    6
#define ACCIIRDEP    6
#define MAGIIRDEP    6

#endif

