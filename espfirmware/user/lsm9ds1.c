#include "lsm9ds1.h"
#include "mystuff.h"
#include <c_types.h>

int SetupLSM()
{
	int r;
	SendStart();
	r = SendByte( 0 );
	if( !r ) { SendStop(); uart0_sendStr("I2C Fault\r\n"); return -2; }
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); uart0_sendStr("AG Fault\r\n"); return -4; }
	SendByte( 0x22 );
	SendByte( 0x81 ); //Reboot
	SendStop();

	SendStart();
	r = SendByte( MA_ADDY );
	if( r ) { SendStop(); uart0_sendStr("MA Fault\r\n"); return -4; }
	SendByte( 0x21 );
	SendByte( 0xC0 ); //Reboot
	SendStop();


	ets_delay_us( 20000 );

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); uart0_sendStr("AG Fault\r\n"); return -4; }
	SendByte( 0x2E );
	SendByte( 0b00000000 ); //Bypass FIFO. //Was: Continuous FIFO mode, FIFO 3 deep.
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); uart0_sendStr("AG Fault\r\n"); return -4; }
	SendByte( 0x22 );
	SendByte( 0b00000110 ); //Auto increment! (Address in lower, too!)
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); uart0_sendStr("AG Fault\r\n"); return -4; }
	SendByte( 0x10 );
	SendByte( 0b10001011 ); //Gyro ODR=238Hz, Cutoff=78Hz, 500DPS
	SendByte( 0b00000000 ); //...
	SendByte( 0b00000000 ); //Highpass = off.
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); uart0_sendStr("AG Fault\r\n"); return -4; }
	SendByte( 0x1E );
	SendByte( 0b00111000 ); //0x1E: Enable Gyro
	SendByte( 0b00111000 ); //0x1F: Turn on Accelerometer
	SendByte( 0b10011000 ); //0x20: Accelerometer, 238Hz, +/- 8g, 105Hz bandwidth
	SendByte( 0b00000000 ); //0x21: Dubious: Disable high-resolution mode.
	SendByte( 0b01000100 ); //0x22: LSB in lower, Autoincrement, push-pull, etc.  DISABLE Block update (UNDO!)
	SendByte( 0b00011000 ); //0x23: Temp in FIFO. DA Timer Enabled, Don't stop on FIFO.
	SendStop();

	SendStart();
	r = SendByte( MA_ADDY );
	if( r ) { SendStop(); uart0_sendStr("MA Fault\r\n"); return -4; }
	SendByte( 0x20 );
	SendByte( 0b11111110 ); //Temp Comp on. Fast ODR (And fast ODR enabled, says higher than 80Hz?)
	SendByte( 0b00000000 ); //+/- 4 Gauss
	SendByte( 0b00000000 ); //I2C, Continuous Conversion
	SendByte( 0b00001100 ); //(LSB first) High performance.
	SendStop();



	return 0;
}

uint16_t LR16( uint8_t nak )
{
	uint16_t ret = GetByte(0);
	ret |= GetByte( nak )<<8;
	return ret;
}


int ReadAGM( int16_t * LINROTMAG )
{
	int r;
	int status = 0;
	int timeout = 0;

	retry:
	SendStart();
	r = SendByte( AG_ADDY );

	//Not sure why, but this seems to happen frequently.
	if( r ) {
		SendStop();
		timeout++;
		if( timeout < 10 )
			goto retry;
		else
			return -81;
	}

	SendByte( 0x17 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	status = GetByte( 1 );
	SendStop();

	//???? What is going on here?
	if( status <= 0 ) { timeout++; if( timeout < 10 ) goto retry; else return -15; }

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); return -2; }
	SendByte( 0x28 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	LINROTMAG[0] = LR16(0);
	LINROTMAG[1] = LR16(0);
	LINROTMAG[2] = LR16(1);
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); return -3; }
	SendByte( 0x18 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	LINROTMAG[3] = LR16(0);
	LINROTMAG[4] = LR16(0);
	LINROTMAG[5] = LR16(1);
	SendStop();
	

	SendStart();
	r = SendByte( MA_ADDY );
	if( r ) { SendStop(); return -4; }
	SendByte( 0x28 );
	SendStop();

	SendStart();
	SendByte( MA_ADDY | 1 );
	LINROTMAG[6] = LR16(0);
	LINROTMAG[7] = LR16(0);
	LINROTMAG[8] = LR16(1);
	SendStop();


	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); return -5; }
	SendByte( 0x15 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	LINROTMAG[9] = LR16(1); //Temp
	SendStop();


	return status;
}

