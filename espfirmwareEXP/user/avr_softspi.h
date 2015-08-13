#ifndef _AVR_SOFTSPI_H
#define _AVR_SOFTSPI_H

#include <c_types.h>

#define GP_AVR_RST 4
#define GP_AVR_MISO 12
#define GP_AVR_MOSI 14
#define GP_AVR_SCK 16 //This is hard coded.

void ICACHE_FLASH_ATTR InitAVRSoftSPI();
void ICACHE_FLASH_ATTR TickAVRSoftSPI( int slow );
int  ICACHE_FLASH_ATTR ProgramAVRFlash( uint8_t * source, uint16_t bytes );
void ICACHE_FLASH_ATTR ResetAVR();


#endif

