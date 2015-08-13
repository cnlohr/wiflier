//Copyright <>< 2015 C. Lohr, See LICENSE file for more information.
//
//TODO:
//  Figure out why data on the LSM is randomly corrupting
//  Make LSM fire at specific interval (probably based on clock)
//  Test Outputs
//  Make Closed loop.
//  Add barometer support.

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>

#define I2CNEEDGETBYTE
#define DPORTNAME B
#define DSDA 0
#define DSCL 1
#define I2CSPEEDBASE .5

void PromptVal( const char * st, uint16_t hv );
uint8_t PushMessage( uint8_t code, const uint8_t * payload );
void SendString( const char * st );
char hexval( uint8_t hv );

#include "static_i2c.h"
#include "lsm9ds1.h"
#include "bmp280.h"

#define TIMEOUT 65535
#define TIMEOUT_RAMP 20000

//Things this does commonly:
//
// * Motor outputs  4b  <
// * Auto Control   13b <
// * ADCH input     1b  > 
// * Bosch BMP280   6b  > (3 Temp, 3 Press)  ** Note contains 26 bytes of parameters.
// * ST LSM9DS1     20b > (2 bytes for each AAAGGGMMMT) ** Note needs 12 bytes to help calibrate.
//
// It is going to have to be able to do realtime closed-loop control on AVR.
//
//
//Bytes back:
//  high nibble = command, low nibble = # of bytes.
//
//Bytes to me:
//  high nibble = command, low nibble = # of bytes.
//
//Reserved commands: 0x00 = no-op.

#define RXBS 64
#define TXBS 256
uint8_t txbuffer[TXBS]; //Us to host.
uint8_t rxbuffer[RXBS];  //Host to us.

volatile uint8_t txhead;
volatile uint8_t txtail;
volatile uint8_t rxhead;
volatile uint8_t rxtail;

register uint8_t spitmp  asm("r8");
register uint8_t spinext asm("r9");
register uint8_t spist   asm("r10");

int16_t lrcur[10];


#define MAG_FILTER 2

//For filtering magnetometer data.
int16_t cummag[3];
uint8_t got_mag_data;

//For sending magnetometer.
int16_t sendmag[3];
uint8_t mag_ready;


//
uint16_t last_agt[7];

uint8_t bmp280dat[6];

int main( )
{
	int r;
	cli();
	CCP = 0xD8;
	CLKPR = 0x00;  //Disable scaling.
//	OSCCAL0 = 0x80; //Changing this will make it not use factory cal.

	ConfigI2C();

	//Now we set up the hardware...
	OCR1BL = 255;
	OCR1AL = 255;
	OCR2BL = 255;
	OCR2AL = 255;

	TCCR1A = _BV(WGM10) | _BV(COM1B1) | _BV(COM1A1) | _BV(COM1B0) | _BV(COM1A0);
	TCCR1B = _BV(WGM12) | _BV(CS10);
	TCCR2A = _BV(WGM20) | _BV(COM2B1) | _BV(COM2A1) | _BV(COM1B0) | _BV(COM1A0);
	TCCR2B = _BV(WGM22) | _BV(CS20);

	//Outputs on TOCC0,1,2,6
	TOCPMSA0 = _BV(TOCC0S0) //Map 1B to TOCC0 (MTR0) (OCR1BL)
			| _BV(TOCC1S0)  //Map 1A to TOCC1 (MTR1) (OCR1AL)
			| _BV(TOCC2S1); //Map 2B to TOCC2 (MTR2) (OCR2BL)
	TOCPMSA1 = _BV(TOCC7S1);//Map 2A to TOCC7 (MTR3) (OCR2AL)

	TOCPMCOE = _BV(TOCC0OE) | _BV(TOCC1OE) | _BV(TOCC2OE) | _BV(TOCC6OE);

	DDRA |= _BV(1) | _BV(2) | _BV(3);
	DDRB |= _BV(7); //Enable outputs

	//Set up ADC to read battery voltage.
	ADMUXA = 0b001101; //1.1v for reading
	ADMUXB = 0; //Vcc for reference
	ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS1);
	ADCSRB = _BV(ADLAR);

	//Set up TWI pullups to help the bus.
	PUEB |= _BV(0) | _BV(1);

	DDRA = 0x0e;


	//Set up the main system ticker.
	TCCR0A = _BV(WGM00) | _BV(WGM01); //Fast PWM (though we don't use the PWM)
	TCCR0B = _BV(WGM02) | _BV(WGM02) | _BV(CS02); //8MHz / 256 = 31.25kHz
	OCR0A = 250; // Use 250 for your divisor, you'll get a 125Hz? Clock  (Should it be 249?)

	//Set up the SPI Bus
	DDRA |= _BV(5); //MISO
	SPCR = 0;
	SPCR |= _BV(SPIE);
	SPCR |= _BV(SPE);

	sei();

	r = InitLSM9DS1();
	PromptVal( PSTR("LSM:"), r );
	r = InitBMP280();
	PromptVal( PSTR("BMP280:"), r );

/*	uint16_t ccv[6];
	r = GetBMPCalVals( 0, (uint8_t*)ccv );
	PromptVal( PSTR("BMPGV:"), r );

	for( i = 0; i < 6; i++ )
	{
		PromptVal( PSTR("LR:"), ccv[i] );
	}
*/
	uint16_t i = 0;
	uint16_t reads = 0;

	while(1)
	{


		//Create magnetometer data at ~30 Hz.
		{
			int16_t lastmag[3];
			if( ReadM(lastmag) == 0 )
			{
				cummag[0] += lastmag[0] >> MAG_FILTER;
				cummag[1] += lastmag[1] >> MAG_FILTER;
				cummag[2] += lastmag[2] >> MAG_FILTER;
				if( ++got_mag_data  == (1<<MAG_FILTER) )
				{
					//PromptVal( PSTR("MAG:"), cummag[0] );	

					sendmag[0] = cummag[0];
					sendmag[1] = cummag[1];
					sendmag[2] = cummag[2];
					got_mag_data = 0;


					//XXX Do something with this data.
					mag_ready = 1;
					cummag[0] = cummag[1] = cummag[2] = 0;

				}
			}
		}

		{
			//int16_t agtdata[7];
			r = ReadAG( last_agt );
			if( r )
			{
				PromptVal( PSTR("AG:"), last_agt[3] );

				//XXX Do something with this data.
				reads++;
			}
		}


		if( TIFR0 & _BV(OCF0A) )
		{
			TIFR0 |= _BV(OCF0A);

			i++;
			if( ( i%125 ) == 0 )
			{
//				PromptVal( PSTR("R:"), reads );
//				PromptVal( PSTR("E:"), r );
//				PromptVal( PSTR("K:"), last_agt[0] );

				//ReadAGM( lrcur );
				//GetBMPTelem( bmp280dat );
			}
		}


//		PromptVal( "LR0:", (bmp280dat[0]<<8) + bmp280dat[1] );
//		PromptVal( "LR:", ccv[i] );
//		PromptVal( PSTR("LR:"), ccv[i] );

//		i++;
//		if( i == 6 ) i = 0;
/*		PromptVal( "LR1:", lrcur[1] );
		PromptVal( "LR2:", lrcur[2] );
		PromptVal( "LR3:", lrcur[3] );
		PromptVal( "LR4:", lrcur[4] );
		PromptVal( "LR5:", lrcur[5] );
		PromptVal( "LR6:", lrcur[6] );
		PromptVal( "LR7:", lrcur[7] );
		PromptVal( "LR8:", lrcur[8] );
		PromptVal( "LR9:", lrcur[9] );*/
	}
}



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////




ISR( SPI_vect, ISR_NAKED )
{
	//Tricky: we have to immediately swap out SPDR's data before we do anything else.
	asm volatile("\t\
		lds r8, 0xB0\n\t\
		sts 0xB0, r9\n\t\
		push r0\n\t\
		in r0,__SREG__\n\t\
		push r0\n\t\
		push r18\n\t\
		push r24\n\t\
		push r25\n\t\
		push r30\n\t\
		push r31\n\t");

	txbuffer[txtail] = 0;

	if( txhead != txtail )
	{
		txtail = (txtail + 1)&(TXBS-1);
	}

	spinext = txbuffer[txtail];

	uint8_t rxfree = (( rxtail - rxhead - 1) + RXBS) & (RXBS-1);

	if( !(spist & 0x80) )
	{
		spist = SPDR & 0x0f;
		if( spist < rxfree )
		{
			spist |= 0x40;
		}
		spist |= 0x80;
	}
	else
	{
		spist--;
		if( (spist & 0x0f) == 0 ) spist = 0;
	}

	if( spist & 0x40 )
	{
		uint8_t rxnext = (rxhead+1)&(RXBS-1);
		rxbuffer[rxnext] = spitmp;
		rxhead = rxnext;
	}

	asm volatile("\n\t\
		pop r31\n\t\
		pop r30\n\t\
		pop r25\n\t\
		pop r24\n\t\
		pop r18\n\t\
		pop r0\n\t\
		out __SREG__,r0\n\t\
		pop r0\n\t\
		reti\n\t");
	
}

//Code = MSN:Command, LSN: Size
uint8_t PushMessage( uint8_t code, const uint8_t * payload )
{
	uint8_t txfree = (( (int16_t)txtail - (int16_t)txhead ) + TXBS - 1) & (TXBS-1);
	uint8_t sizeto = code & 0x0f;
	uint8_t nh = txhead;

	if( txfree > (sizeto+1) )
	{
		nh = (nh+1)&(TXBS-1);
		txbuffer[nh] = code;

		while( sizeto-- )
		{
			nh = (nh+1)&(TXBS-1);
			txbuffer[nh] = *(payload++);
		}
		txhead = nh;
		return (code & 0x0f)+1;
	}
	return 0;
}

void SendString_RAM( const char * st )
{
	int stl = strlen( st );

	while( stl )
	{
		int tss = ( stl >= 16 )?15:stl;
		PushMessage( 0x10 | tss, (uint8_t*)st );
		stl -= tss;
		st += tss;
	}

}


void SendString( const char * st )
{
	char pak[16];
	int stl = strlen_P( st );

	while( stl )
	{
		int tss = ( stl >= 16 )?15:stl;
		memcpy_P( pak, st, tss );
		PushMessage( 0x10 | tss, (uint8_t*)pak );
		stl -= tss;
		st += tss;
	}
}

char hexval( uint8_t hv )
{
	hv &= 0x0f;
	if( hv <= 9 )
		return '0' + hv;
	else
		return 'a' + hv - 10;
}

void PromptVal( const char * st, uint16_t hv )
{
	int stc = strlen_P( st );
	char st2[16];
	if( stc > 11 ) stc = 11;

	memcpy_P( st2, st, stc );

	st2[stc++] = hexval( hv >> 12 );
	st2[stc++] = hexval( hv >> 8 );
	st2[stc++] = hexval( hv >> 4 );
	st2[stc++] = hexval( hv );
	st2[stc++] = 0;
	SendString_RAM( st2 );
}



