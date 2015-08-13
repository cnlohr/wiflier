// This is intended to be included in the main .c, after static_i2c.h

#define BMP280_ADDY 0xEC

int InitBMP280()
{
	int r;

	SendStart();
	r = SendByte( 0 );
	if( !r ) { SendStop(); SendString(PSTR("I2C Fault")); return -2; }
	SendStop();

	SendStart();
	r = SendByte( BMP280_ADDY );
	if( r ) { SendStop(); SendString(PSTR("BMP Fault")); return -3; }
	SendByte( 0xE0 );
	SendByte( 0xB6 ); //Reboot
	SendStop();

	SendStart();
	r = SendByte( BMP280_ADDY );
	if( r ) { SendStop(); SendString(PSTR("BMP ID Fault")); return -4; }
	SendByte( 0xD0 );
	SendStop();

	SendStart();
	r = SendByte( BMP280_ADDY | 1 );
	if( r ) { SendStop(); SendString(PSTR("BMP Fault")); return -5; }
	if( (r=GetByte(1)) != 0x58 ) { SendStop(); PromptVal(PSTR("BMPID"),r); return -6; }
	SendStop();


	//Datasheet recommends setting up for: 
	// osrs_p = x16      osrs_p = 101
	// osrs_t = x2       osrs_t = 010
	// filter iir = 16              (though I question this)
	// ODR = 26.3        
	// Mode = Normal     mode = 11
	// Standby = 0       t_sb = 000
	

	SendStart();
	r = SendByte( BMP280_ADDY );
	if( r ) { SendStop(); SendString(PSTR("BMP ID Fault")); return -4; }
	SendByte( 0xF4 );
	SendByte( 0b01010111 ); //osrs_t osrs_p mode


	//XXX Filter may be incorrect
	//t_sb filter spi3w_en
	SendByte( 0b00010100 );

	SendStop();


	return 0;
}

uint8_t BMPGetStatus()
{
	uint8_t ret = 0;
	SendStart();
	if( SendByte( BMP280_ADDY ) ) { SendStop(); SendString(PSTR("BMP STAT")); return 0xff; }
	SendByte( 0xF3 );
	SendStop();

	SendStart();
	if( SendByte( BMP280_ADDY | 1 ) ) { SendStop(); SendString(PSTR("BMP STAT")); return 0xff; }
	ret = GetByte(1);
	SendStop();
	return ret;
}


//Which = 0 or 12.  Returns 12 values.
int GetBMPCalVals( int offset, uint8_t * vals )
{
	int i, r;
	i = 0;
	for( i = 0; i < 100; i++ )
	{
		if( !(BMPGetStatus() & 1 )) break;
		_delay_us(10);
	}
	if( i == 10 )
	{
		SendString(PSTR("BMP TO")); return -1;
	}
/*
retry:
	SendStart();
	if( SendByte( BMP280_ADDY ) ) { SendStop();  }
	SendByte( 0xF3 );
	SendStop();

	SendStart();
	r = SendByte( BMP280_ADDY | 1 );
	if( r ) { SendStop(); SendString(PSTR("BMP ST Fault")); return -5; }
	if( GetByte(1) & 1 ) {
		i++;
		SendStop();
		if( i > 100 )
		{
			SendString(PSTR("BMP ST TO"));
			return -9;
		}
		goto retry;
	}
	SendStop();

*/
	SendStart();
	r = SendByte( BMP280_ADDY );
	if( r ) { SendStop(); SendString(PSTR("BMP CC Fault")); return -4; }
	SendByte( 0x88 + offset );
	SendStop();

	SendStart();
	r = SendByte( BMP280_ADDY | 1 );
	if( r ) { SendStop(); SendString(PSTR("BMP CC Fault")); return -5; }
	for( i = 0; i < 11; i++ )
		vals[i] = GetByte(0);
	vals[11] = GetByte(1);
	SendStop();

	return 0;
}

//Returns 6 values.  MSB first for Pressure (3 bytes) then Temp (3 bytes)
int GetBMPTelem( uint8_t * vals )
{
	uint8_t i;

	SendStart();
	if( SendByte( BMP280_ADDY ) ) { SendStop(); SendString(PSTR("BMP BB Fault")); return -4; }
	SendByte( 0xF7 );
	SendStop();

	SendStart();
	if( SendByte( BMP280_ADDY | 1 ) ) { SendStop(); SendString(PSTR("BMP BB Fault")); return -5; }
	for( i = 0; i < 5; i++ )
		vals[i] = GetByte(0);
	vals[5] = GetByte(1);
	SendStop();

	return 0;
}


