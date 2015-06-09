#ifndef _BMP085_H
#define _BMP085_H

#include "i2c.h"

#define BMP_ADDY 0xEE

int SetupBMP085();
uint32_t GetBMP085();
uint16_t GetBMPTemp();
void DoBMPCalcs();

extern int16_t bmp_degc;
extern uint32_t bmp_pa;


#endif

