#ifndef _LSM9DS1_H
#define _LSM9DS1_H

#include "i2c.h"

//Currently set up to target 150 Hz?

//These are 8-bit addresses
#define AG_ADDY 0xD4
#define MA_ADDY 0x38

//0 = good, nonzero = bad
int SetupLSM();

//0 = good, nonzero = bad
//Needs 10 int16_t's.  LINEAR, ROTATION, MAG, TEMP
int ReadAGM( int16_t * LINROTMAG );

#endif

