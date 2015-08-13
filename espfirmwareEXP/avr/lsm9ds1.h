// This is intended to be included in the main .c, after static_i2c.h

#define AG_ADDY 0xD4
#define MA_ADDY 0x38

int InitLSM9DS1()
{
	int r;
	SendStart();
	r = SendByte( 0 );
	if( !r ) { SendStop(); SendString(PSTR("I2C Fault")); return -2; }
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); SendString(PSTR("AG Fault")); return -4; }
	SendByte( 0x22 );
	SendByte( 0x81 ); //Reboot
	SendStop();

	SendStart();
	r = SendByte( MA_ADDY );
	if( r ) { SendStop(); SendString(PSTR("MA Fault")); return -4; }
	SendByte( 0x21 );
	SendByte( 0xC0 ); //Reboot
	SendStop();


	_delay_us( 20000 );


	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); SendString(PSTR("AG FaultB")); return -4; }
	SendByte( 0x22 );
	SendByte( 0b00000110 ); //Auto increment! (Address in lower, too!)
	SendStop();


	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); SendString(PSTR("AG FaultA")); return -4; }
	SendByte( 0x2E );
	SendByte( 0b11000011 ); //FIFO mode, 3 deep.
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); SendString(PSTR("AG FaultC")); return -4; }
	SendByte( 0x10 );
	SendByte( 0b01101011 ); //Gyro ODR=119Hz, Cutoff=30Hz, 500DPS
	SendByte( 0b00000000 ); //XXX Consider making outsel 10, to enable LPF2
	SendByte( 0b00000000 ); //Highpass = off.
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); SendString(PSTR("AG Fault")); return -4; }
	SendByte( 0x1E );
	SendByte( 0b00111000 ); //0x1E: Enable Gyro
	SendByte( 0b00111000 ); //0x1F: Turn on Accelerometer
	SendByte( 0b01111000 ); //0x20: Accelerometer, 119Hz, +/- 8g, 50Hz bandwidth
	SendByte( 0b00000000 ); //0x21: Dubious: Disable high-resolution mode.
	SendByte( 0b01000100 ); //0x22: LSB in lower, Autoincrement, push-pull, etc.  TODO: Block update?
	SendByte( 0b00011010 ); //0x23: Temp in FIFO. DA Timer Enabled, FIFO Enabled Don't stop on FIFO.
	SendStop();

	SendStart();
	r = SendByte( MA_ADDY );
	if( r ) { SendStop(); SendString(PSTR("MA Fault")); return -4; }
	SendByte( 0x20 );
	SendByte( 0b11111110 ); //Temp Comp on. None of this register makes any sense.
	SendByte( 0b00000000 ); //+/- 4 Gauss
	SendByte( 0b00000000 ); //I2C, Continuous Conversion
	SendByte( 0b00001100 ); //(LSB first) High performance.
	SendByte( 0b01000000 ); //BDU Enabled.
	SendStop();

	return 0;
}



uint16_t LSM9LR16( uint8_t nak )
{
	uint16_t ret = GetByte(0);
	ret |= GetByte( nak )<<8;
	return ret;
}


#if 0

//This function will not be maintained.
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
	LINROTMAG[0] = LSM9LR16(0);
	LINROTMAG[1] = LSM9LR16(0);
	LINROTMAG[2] = LSM9LR16(1);
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); return -3; }
	SendByte( 0x18 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	LINROTMAG[3] = LSM9LR16(0);
	LINROTMAG[4] = LSM9LR16(0);
	LINROTMAG[5] = LSM9LR16(1);
	SendStop();
	

	SendStart();
	r = SendByte( MA_ADDY );
	if( r ) { SendStop(); return -4; }
	SendByte( 0x28 );
	SendStop();

	SendStart();
	SendByte( MA_ADDY | 1 );
	LINROTMAG[6] = LSM9LR16(0);
	LINROTMAG[7] = LSM9LR16(0);
	LINROTMAG[8] = LSM9LR16(1);
	SendStop();


	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); return -5; }
	SendByte( 0x15 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	LINROTMAG[9] = LSM9LR16(1); //Temp
	SendStop();


	return status;
}
#endif

//This seems to be pretty solid now.
int ReadM( int16_t * MAG )
{
	SendStart();
	uint8_t r = SendByte( MA_ADDY );
	if( r ) { SendStop(); return -4; }
	SendByte( 0x27 );
	SendStop();

	SendStart();
	SendByte( MA_ADDY | 1 );
	r = GetByte( 0 );

	if( !(r&0x08) )
	{
		//No new data ready.
		GetByte( 1 ); //Need a nac.
		SendStop();
		return -2;
	}

	MAG[0] = LSM9LR16(0);
	MAG[1] = LSM9LR16(0);
	MAG[2] = LSM9LR16(1);
	SendStop();
	return 0;
}

//Returns # of entries left in fifo if ok.
int ReadAG( int16_t * LINROT )
{
	int r;
	int status = 0;

/*	retry:
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
*/
	SendStart();
	r = SendByte( AG_ADDY );
	SendByte( 0x2f );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	status = GetByte( 1 );
	SendStop();

	if( status == 0 ) return 0;

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); return -2; }
	SendByte( 0x28 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	LINROT[0] = LSM9LR16(0);
	LINROT[1] = LSM9LR16(0);
	LINROT[2] = LSM9LR16(1);
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); return -3; }
	SendByte( 0x18 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	LINROT[3] = LSM9LR16(0);
	LINROT[4] = LSM9LR16(0);
	LINROT[5] = LSM9LR16(1);
	SendStop();

	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); return -5; }
	SendByte( 0x15 );
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	LINROT[6] = LSM9LR16(1); //Temp
	SendStop();


	return status;
}
