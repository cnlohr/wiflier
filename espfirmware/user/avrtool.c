#include "avrtool.h"
#include "i2c.h"
#include "mystuff.h"

int SetupAVRTool()
{
	int c = 0;

	SendStart();
	c |= SendByte( 0x20 );
	c |= SendByte( 0x01 );
	c |= SendByte( 0x00 );
	c |= SendByte( 0x00 );
	c |= SendByte( 0x00 );
	c |= SendByte( 0x00 );
	SendStop();

	return c;
}

int RunAVRTool( unsigned char * motors )
{
	int adc;
	int ca = 0, cb = 0;
	int tries = 0;
retry:
	SendStart();
	ca |= SendByte( 0x20 );
	ca |= SendByte( 31 );

	//I have no idea why we have to do this.  If we don't have some extra 0's here, the motors will go crazy and the AVR will lose its mind.
	ca |= SendByte( 0x00 ); //unused.
	ca |= SendByte( 0x00 ); //unused.

	if( ca )
	{
		SendStop();
		tries++;
		ca = 0;
		if( tries < 3 )
			goto retry;
		return 2;
	}

	ca |= SendByte( motors[0] );
	ca |= SendByte( motors[1] );
	ca |= SendByte( motors[2] );
	ca |= SendByte( motors[3] );
	SendStop();


	SendStart();
	cb |= SendByte( 0x21 );
	adc = GetByte( 1 );
	SendStop();

	//1.1 / 5 * 256 = 56.32
	//56.32 / 1.1 = 5 * 256
	//(1.1*256) / (56.32) = 5
	//(1.1*256*5) / ADC = voltage in centivolts.

	//ADC = 1.1v reference with bus as reference.

	if( cb ) return 3;
	if( adc == 0xff || adc == 0 )
		return 1;
	else
		return 28444 / adc;
}
