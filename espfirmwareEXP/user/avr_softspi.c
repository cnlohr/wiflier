//AVR Programming and serial communications interface, loosely based off of AVR910.

#include "avr_softspi.h"
#include <gpio.h>
#include <eagle_soc.h>
#include <mystuff.h>

#define _delay_us(x) ets_delay_us(x)
void _delay_ms( unsigned int x )
{
	while(x--)
		ets_delay_us(1000);
}

#define AVRSCKON  {(*((volatile uint32_t *)ETS_UNCACHED_ADDR(RTC_GPIO_OUT))) |= 1;}
#define AVRSCKOFF {(*((volatile uint32_t *)ETS_UNCACHED_ADDR(RTC_GPIO_OUT))) &= 0xfffffffe;}

//Consider optimizing. GPIO_... functions are slow.
#define AVRSETRST( x ) { GPIO_OUTPUT_SET( GPIO_ID_PIN( GP_AVR_RST ), x ); }
#define AVRSETMOSI( x ) { GPIO_OUTPUT_SET( GPIO_ID_PIN( GP_AVR_MOSI ), x ); }
#define AVRGETMISO GPIO_INPUT_GET( GPIO_ID_PIN( GP_AVR_MISO ) )

//NOTES: When reading, data is clocked on the falling edge of SCK
//ESP8266 has a LSB first endian.  The bytes are MSB first.  We're just going to
//ignore that fact.

#define ERASE_TIMEOUT 25 //Roughly in MS.
#define FLASH_PAGE_SIZE_WORDS 8

static void avrdelay()
{
	uint8_t i;
	for( i = 0; i < 10; i++ ) asm("nop");
}

static uint8_t AVRSR( uint8_t data )
{
	uint8_t ret = 0;
	uint8_t i = 0x80;

	_delay_us(1);
	for( ; i != 0; i>>=1 )
	{
		AVRSETMOSI( ((i&data)!=0) )
		avrdelay();
		AVRSCKON
		avrdelay();
		ret |= AVRGETMISO?i:0;
		AVRSCKOFF
	}

	return ret;
}

static uint32_t AVRSR4( uint32_t data )
{
	uint32_t ret = 0;
	uint32_t i = 0x80000000;
	for( ; i != 0; i>>=1 )
	{
		AVRSETMOSI( ((i&data)!=0) )
		_delay_us(2);
		AVRSCKON
		_delay_us(2);
		ret |= AVRGETMISO?i:0;
		AVRSCKOFF
	}

	_delay_us(10); //XXX TODO Try to reduce this.

	return ret;
}

static int ICACHE_FLASH_ATTR EnterAVRProgramMode()
{
	uint32_t rr;

	AVRSCKOFF;
	_delay_us(5);
	AVRSETRST(1);
	_delay_us(20);
	AVRSETRST(0);
	_delay_ms(20); //XXX TODO try to reduce this.

	rr = AVRSR4( 0xAC530000 );
	if( ( rr & 0x0000ff00 ) != 0x00005300 )
	{
		printf( "RR: %08x\n", rr );
		return 0xffff0000;
	}

	if( ( rr = AVRSR4( 0x30000000 ) & 0xff ) != 0x1e )
	{
		return 0xffff1000 | ( rr & 0xff );
	}

	if( ( rr = AVRSR4( 0x30000100 ) & 0xff ) != 0x93 ) //0x92 = ATTiny441 0x93 = ATTiny841
	{
		return 0xffff2000 | ( rr & 0xff );
	}

	if( ( rr = AVRSR4( 0x30000200 ) & 0xff ) != 0x15 )
	{
		return 0xffff3000 | ( rr & 0xff );
	}

	return 0;
}

static int ICACHE_FLASH_ATTR WaitForAVR()
{
	int i;
	for( i = 0; i < ERASE_TIMEOUT; i++ )
	{
		if( ( AVRSR4(0xF0000000) & 1 )  == 0 ) return 0;
		_delay_ms(1);
	}
	return 0xFFFFBEEF;
}


static int ICACHE_FLASH_ATTR EraseAVR()
{
	int i;
	int r = AVRSR4( 0xAC800000 );

	if( WaitForAVR() ) return 0xFFFF5000;

	return 0;
}


void ICACHE_FLASH_ATTR ResetAVR()
{
	AVRSETRST(0);
	_delay_us(10);
	AVRSETRST(1);
	_delay_ms(20);
}

int ICACHE_FLASH_ATTR ProgramAVRFlash( uint8_t * source, uint16_t bytes )
{
	int i;
	int r;

	r = EnterAVRProgramMode(); 	if( r ) { printf( "Enter Program Mode Fail.\n" ); goto fail; }
	r = EraseAVR(); 			if( r ) { printf( "Erase fail.\n" ); goto fail; }
	printf( "Programming AVR %p:%d\n", source, bytes );
	int bytesin = 0;
	int blockno = 0;

	while( bytes - bytesin > 0 )
	{
		uint8_t buffer[16];
		int wordsthis = (bytes - bytesin + 1) / 2;
		if( wordsthis > 8 ) wordsthis = 8;

		ets_memcpy( buffer, &source[bytesin], 16 );

		for( i = 0 ; i < wordsthis; i++ )
		{
			AVRSR4( 0x40000000 | (i << 8) | buffer[i*2+0] );
			AVRSR4( 0x48000000 | (i << 8) | buffer[i*2+1] );
			//printf( "%02x%02x ",buffer[i*2+0],buffer[i*2+1] );
		}

		r = AVRSR4( 0x4C000000 | (blockno<<(8+3)) );
		if( WaitForAVR() ) return -9;

		blockno++;
		bytesin+=16;
	}

//#define AVR_VERIFY
#ifdef AVR_VERIFY
	printf("\nReading\n" );
	for( i = 0; i < (bytes+1)/2; i++ )
	{
		uint8_t ir = AVRSR4( 0x20000000 | ((i)<<8 ) )&0xff;
		printf( "%02x", ir );
		ir = AVRSR4( 0x28000000 | ((i)<<8) )&0xff;
		printf( "%02x ", ir );
	}
#endif

/*

	while( bytes - bytesin > 0 )
	{
		int wordsthis = (bytes - bytesin + 1) / 2;
		if( wordsthis > 8 ) wordsthis = 8;

		for( i = 0 ; i < wordsthis; i++ )
		{
			AVRSR4( 0x40000000 | (i << 8) | source[i*2+0] );
			AVRSR4( 0x48000000 | (i << 8) );
		}
	for( i = 0; i < 8; i++ )
	{
		printf( "B: %08x\n", AVRSR4( 0x40000089 + (i << 8) ) ); //LSB first.
		printf( "A: %08x\n", AVRSR4( 0x480000AB + (i << 8) ) );
	}
	printf( "C: %08x\n", AVRSR4( 0x4C000800 ) ); //2nd block

	r = WaitForAVR(); if( r ) goto fail;
	printf( "WF: %d\n",  );
	
	for( i = 0; i < 10; i++ )
	{
		printf( "E: %08x\n", AVRSR4( 0x20000000 + (i<<8) ) );
		printf( "D: %08x\n", AVRSR4( 0x28000000 + (i<<8) ) );
	}	
*/
	printf( "Reset AVR.\n" );
	AVRSETRST(1);
	_delay_ms(20);

	return 0;
fail:
	ets_wdt_enable();
	printf( "Program fail: %08x\n", r );
	return -1;
}

void InitAVRSoftSPI()
{
	//Tricky: AVRSCK = GPIO16 = Special output.
	WRITE_PERI_REG(PAD_XPD_DCDC_CONF,    (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC to output rtc_gpio0
	WRITE_PERI_REG(RTC_GPIO_CONF,    (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable
    WRITE_PERI_REG(RTC_GPIO_ENABLE,(READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);	//out enable

	GPIO_OUTPUT_SET(GPIO_ID_PIN(GP_AVR_RST), 0 );
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(GP_AVR_MISO) );
	GPIO_OUTPUT_SET(GPIO_ID_PIN(GP_AVR_MOSI), 1 );
	PIN_FUNC_SELECT( PERIPHS_IO_MUX_MTMS_U, 3);
	PIN_FUNC_SELECT( PERIPHS_IO_MUX_MTDI_U, 3); 
	AVRSETRST(0);
	_delay_us(30);
	AVRSETRST(1);
	_delay_ms(20);
}

int wason = 0;

void TickAVRSoftSPI( int slow )
{
//	if( !slow ) return;

//	int ir = AVRSR( 'T' );

	int t;

	for( t = 0; t < 10; t++ )
	{
		int ir = AVRSR( 0 );
		if( ir )
		{
			int j = 0;
			printf( "%02x: ", ir );
			ir &= 0x0f;
			for( j = 0; j < ir; j++ )
			{
				printf( "%c", AVRSR( 0 ) );
			}
			printf( "\n" );
		}
		else
		{
			break;
		}
	}

//	printf( "\n" );
/*	static int frame;
	if( !slow ) return;

	frame++;

	if( wason )
	{
		AVRSETMOSI(1)
	}
	else
	{
		AVRSETMOSI(0)
	}
	
	printf( "%d %d\n", wason, AVRGETMISO );
	if( ( frame & 0xf ) == 0 )
	{
		wason = !wason;
		ProgramFlash();
	}*/
}


