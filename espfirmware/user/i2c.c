#include "i2c.h"

//#define REMAP(x) GPIO_ID_PIN(x)
#define REMAP(x) x

void ICACHE_FLASH_ATTR ConfigI2C()
{
	GPIO_DIS_OUTPUT(REMAP(I2CSDA));
	GPIO_DIS_OUTPUT(REMAP(I2CSCL));
}

void SendStart()
{
	I2CDELAY
	I2CDELAY
	GPIO_OUTPUT_SET(REMAP(I2CSDA), 0);
	I2CDELAY
	GPIO_OUTPUT_SET(REMAP(I2CSCL), 0);
}

void SendStop()
{
	I2CDELAY
	GPIO_OUTPUT_SET(REMAP(I2CSDA), 0);  //May or may not be done.
	GPIO_OUTPUT_SET(REMAP(I2CSCL), 0);  //Should already be done.

	I2CDELAY
	GPIO_DIS_OUTPUT(REMAP(I2CSCL));
	I2CDELAY
	GPIO_DIS_OUTPUT(REMAP(I2CSDA));
	I2CDELAY
	I2CDELAY
}

//Return nonzero on failure.
unsigned char SendByte( unsigned char data )
{
	unsigned char i;
	//Assume we are in a started state (DSCL = 0 & DSDA = 0)
/*
	DPORT |= _BV(DSDA);
	DDDR |= _BV(DSDA);
*/
	for( i = 0; i < 8; i++ )
	{
		I2CDELAY

		if( data & 0x80 )
			GPIO_DIS_OUTPUT(REMAP(I2CSDA));
		else
			GPIO_OUTPUT_SET(REMAP(I2CSDA), 0);

		data<<=1;

		I2CDELAY

		GPIO_DIS_OUTPUT(REMAP(I2CSCL));

		I2CDELAY
		I2CDELAY

		GPIO_OUTPUT_SET(REMAP(I2CSCL), 0);
	}

	//Immediately after sending last bit, open up DDDR for control.
	GPIO_DIS_OUTPUT(REMAP(I2CSDA));
	I2CDELAY
	GPIO_DIS_OUTPUT(REMAP(I2CSCL));
	I2CDELAY
	I2CDELAY
	i = GPIO_INPUT_GET( REMAP(I2CSDA));
	GPIO_OUTPUT_SET(REMAP(I2CSCL), 0);
	I2CDELAY

//  Leave in open collector.

	return (i)?1:0;
}

unsigned char GetByte( uint8_t send_nak )
{
	unsigned char i;
	unsigned char ret = 0;

	GPIO_DIS_OUTPUT(REMAP(I2CSDA));

	for( i = 0; i < 8; i++ )
	{
		I2CDELAY
		GPIO_DIS_OUTPUT(REMAP(I2CSCL));
		I2CDELAY
		I2CDELAY

		ret<<=1;

		if( GPIO_INPUT_GET( REMAP(I2CSDA)) )
			ret |= 1;

		GPIO_OUTPUT_SET(REMAP(I2CSCL), 0);
		I2CDELAY
	}

	//Send ack.
	if( send_nak )
	{
	}
	else
	{
		GPIO_OUTPUT_SET(REMAP(I2CSDA), 0);
	}
	I2CDELAY
	GPIO_DIS_OUTPUT(REMAP(I2CSCL));
	I2CDELAY
	I2CDELAY
	I2CDELAY
	GPIO_OUTPUT_SET(REMAP(I2CSCL), 0);
	I2CDELAY
	GPIO_DIS_OUTPUT(REMAP(I2CSDA));

	return ret;
}

void my_i2c_delay()
{
	//These are assuming 1.5k
	//If i = 2, there are a lot of errors, with 1.5k pullups on the I2C.
	//If i = 3, the AVR can't keep up.
	//If i = 4, things are hokey.
	//If i = 5, things are even worse
	//If i = 6..8, splended, but AVR is super flakey.
	//If i = 9, AVR is almost
	//If i = 10, AVR is solid. Let's go above there...
	// I like i = 15 @2.7k
	uint8_t i = 15;
	do
	{
		asm("nop");	
	} while( i-- );
}


