#include "bmp085.h"

static int16_t ac1;
static int16_t ac2;
static int16_t ac3;
static uint16_t ac4;
static uint16_t ac5;
static uint16_t ac6;
static int16_t b1;
static int16_t b2;
static int16_t mb;
static int16_t mc;
static int16_t md;

static uint32_t UT;
static uint32_t UP;

int16_t bmp_degc;
uint32_t bmp_pa;

#define OSS 3

#if OSS == 3
#define GOCODE 0xF4
#elif OSS == 2
#define GOCODE 0xB4
#elif OSS == 1
#define GOCODE 0x74
#elif OSS == 0
#define GOCODE 0x34
#else
#error invalid OSS
#endif

static uint16_t Read16( )
{
	uint16_t ret;
	ret = GetByte( 0 );
	ret = ( ret<<8 ) | GetByte(0);
	return ret;
}


int SetupBMP085()
{
	int r;
	SendStart();
	r = SendByte( 0 );
	if( !r ) { SendStop(); uart0_sendStr("I2C Fault\r\n"); return -2; }
	SendStop();

	SendStart();
	r = SendByte( BMP_ADDY );
	if( r ) { SendStop(); uart0_sendStr("BMP Fault\r\n"); return -4; }
	SendByte( 0xAA );
	SendStop();

	SendStart();
	SendByte( BMP_ADDY | 1 );
	ac1 = Read16( );
	ac2 = Read16( );
	ac3 = Read16( );
	ac4 = Read16( );
	ac5 = Read16( );
	ac6 = Read16( );

	b1 = Read16( );
	b2 = Read16( );
	mb = Read16( );
	mc = Read16( );
	md = Read16( );
	GetByte( 1 ) ;
	SendStop();

	SendStart();
	r = SendByte( BMP_ADDY );
	if( r ) { SendStop(); uart0_sendStr("BMP Fault\r\n"); return -4; }
	SendByte( 0xf4 );
	SendByte( 0xf4 ); //Pressure calculation
	SendStop();

	return 0;
}

uint32_t GetBMP085()
{
	uint32_t ret;
	SendStart();
	SendByte( BMP_ADDY );
	SendByte( 0xF6 );
	SendStop();

	SendStart();
	SendByte( BMP_ADDY | 1 );
	ret = GetByte( 0 );
	ret = (ret<<8) | GetByte( 0 );
	ret = (ret<<8) | GetByte( 1 );
	SendStop();

	UP = ret>>(8-OSS);

	SendStart();
	SendByte( BMP_ADDY );
	SendByte( 0xf4 );
	SendByte( 0x2e );
	SendStop();

	return ret;
}

uint16_t GetBMPTemp()
{
	uint32_t ret;
	SendStart();
	SendByte( BMP_ADDY );
	SendByte( 0xF6 );
	SendStop();

	SendStart();
	SendByte( BMP_ADDY | 1 );
	ret = GetByte( 0 );
	ret = (ret<<8) | GetByte( 1 );
	SendStop();

	SendStart();
	SendByte( BMP_ADDY );
	SendByte( 0xf4 );
	SendByte( 0xf4 );
	SendStop();

	UT = ret;

	return ret;

}

void DoBMPCalcs()
{
	char buffer[400];
	char * buffend = buffer;

	int32  tval, pval;
	int32  x1, x2, x3, b3, b5, b6, p;
	uint32  b4, b7;

	x1 = (UT - ac6) * ac5 >> 15;
	x2 = ((int32 ) mc << 11) / (x1 + md);
	b5 = x1 + x2;
	tval = (b5 + 8) >> 4;
	bmp_degc = tval;

	uint8 oss = OSS;

	b6 = b5 - 4000;
//	buffend += ets_sprintf( buffend, "b6 = %d UP=%d UT=%d oss=%d\n", b6, UP, UT, oss );
	x1 = (b2 * (b6 * b6 >> 12)) >> 11; 
//	buffend += ets_sprintf( buffend, "x1 = %d\n", x1 );
	x2 = ac2 * b6 >> 11;
//	buffend += ets_sprintf( buffend, "x2 = %d\n", x2 );
	x3 = x1 + x2;
//	buffend += ets_sprintf( buffend, "x3 = %d\n", x3 );
	b3 = ((((((int32)ac1) << 2 ) + x3)<<oss) + 2) >> 2;
//	buffend += ets_sprintf( buffend, "b3 = %d\n", b3 );
	x1 = ac3 * b6 >> 13;
//	buffend += ets_sprintf( buffend, "x1 = %d\n", x1 );
	x2 = (b1 * (b6 * b6 >> 12)) >> 16;
//	buffend += ets_sprintf( buffend, "x2 = %d\n", x2 );
	x3 = ((x1 + x2) + 2) >> 2;
//	buffend += ets_sprintf( buffend, "x3 = %d\n", x3 );
	b4 = (ac4 * (uint32 ) (x3 + 32768)) >> 15;
//	buffend += ets_sprintf( buffend, "b4 = %d\n", b4 );
	b7 = ((uint32 ) UP - b3) * (50000 >> oss);
//	buffend += ets_sprintf( buffend, "b7 = %u\n", b7 );
	p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
//	buffend += ets_sprintf( buffend, "p = %d\n", p );

	x1 = (p >> 8) * (p >> 8);
//	buffend += ets_sprintf( buffend, "x1 = %d\n", x1 );
	x1 = (x1 * 3038) >> 16;
//	buffend += ets_sprintf( buffend, "x1 = %d\n", x1 );
	x2 = (-7357 * p) >> 16;
//	buffend += ets_sprintf( buffend, "x2 = %d\n", x2 );
	bmp_pa = pval = p + ((x1 + x2 + 3791) >> 4);
//	buffend += ets_sprintf( buffend, "pa = %d\n", pa );

	//uart0_sendStr(buffer); //Just a marker so we know we're alive.

}


