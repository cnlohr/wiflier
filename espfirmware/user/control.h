#ifndef _CONTROL_H
#define _CONTROL_H

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"


struct SaveSector
{
	int32_t gyrocenter[3];
	int32_t acccenter[3];
	int32_t	accelmin[3];
	int32_t	accelmax[3];
	int32_t	magmin[3];
	int32_t	magmax[3];
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

#define MAGIIRDEP 7
#define ACCIIRDEP    7
#define GYRIIRDEP    9

#define GYRIIRDIV (1./512.)
#define ACCIIRDIV (1./128.)
#define ACCUNITS .001
#define ACCIIRMUX (128.)
#define MAGIIRDIV (1./128.)

#define GYROUNITS .001

#endif

