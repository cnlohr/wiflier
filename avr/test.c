#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define TIMEOUT 65535
#define TIMEOUT_RAMP 20000

volatile uint8_t operation = 0;
uint8_t state = 0;
uint8_t address = 0;
uint16_t timeout = 0;

int main( )
{
	CCP = 0xD8;
	CLKPR = 0x00;  //Disable scaling.
	OSCCAL0 = 0xFF;

	//We don't use interrupts in this program.
	cli();
	TWSCRA = _BV(TWDIE) | //Data Interrupt Flag
//		_BV( TWSHE ) | //TWI SDA Hold Time Enable
		_BV( TWASIE ) | //Stop+Address Flag
		_BV( TWEN ) | //Enable TWI.
		_BV( TWSIE ) | //Stop flag
		_BV( TWSME ) | //Smart mode.
		0;
	TWSCRB = _BV(TWHNM) | //High noise mode.
		0;

	TWSA = 0x20; //Slave address.
	TWSAM = 0; //Don't mask.

	//Now we set up the hardware...
	OCR1BL = 255;
	OCR1AL = 255;
	OCR2BL = 255;
	OCR0B = 255;

	TCCR1A = _BV(WGM10) | _BV(COM1B1) | _BV(COM1A1) | _BV(COM1B0) | _BV(COM1A0);
	TCCR1B = _BV(WGM12) | _BV(CS11);
	TCCR2A = _BV(WGM20) | _BV(COM2B1) | _BV(COM2B0);
	TCCR2B = _BV(WGM22) | _BV(CS21);
	TCCR0A = _BV(COM0B1) | _BV(COM0B0) | _BV(WGM00) | _BV(WGM01);
	TCCR0B = _BV(CS01);

	//Outputs on TOCC0,1,2,6
	TOCPMSA0 = _BV(TOCC0S0) //Map 1B to TOCC0 (MTR0) (OCR1BL)
			| _BV(TOCC1S0) //Map 1A to TOCC1 (MTR1) (OCR1AL)
			| _BV(TOCC2S1) //Map 2B to TOCC2 (MTR2) (OCR2BL)
			| 0;
	TOCPMSA1 = 0; //Map OC0B to TOCC6 (MTR3)  (OCR0B)
	TOCPMCOE = _BV(TOCC0OE) | _BV(TOCC1OE) | _BV(TOCC2OE) | _BV(TOCC6OE);

	DDRA = _BV(1) | _BV(2) | _BV(3) | _BV(7); //Enable outputs

	//Set up ADC to read battery voltage.
	ADMUXA = 0b001101; //1.1v for reading
	ADMUXB = 0; //Vcc for reference
	ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS1);
	ADCSRB = _BV(ADLAR);

	//Set up TWI pullups to help the bus.
	PUEA |= _BV(4) | _BV(6);

	//UUGGGHHH I wish this was interrupt driven, but I can't figure out why the ATTiny441 is locking
	//the bus when I do that.
	while(1)
	{
		if( TWSSRA & _BV(TWC) ) TWSSRA |= _BV(TWC);
		if( TWSSRA & _BV(TWBE) ) TWSSRA |= _BV(TWBE);
		if( timeout == 0 )
		{
			if( OCR1BL < 255 ) OCR1BL++;
			if( OCR1AL < 255 ) OCR1AL++;
			if( OCR2BL < 255 ) OCR2BL++;
			if( OCR0B < 255 ) OCR0B++;
			timeout = TIMEOUT_RAMP;
		}
		else
		{
			timeout--;
		}
		if( TWSSRA & _BV(TWDIF) )
		{
			if( TWSSRA & _BV(TWDIR) )
			{
				switch( address )
				{
				case 1:  TWSD = 255-OCR1BL; break;
				case 2:  TWSD = 255-OCR1AL; break;
				case 3:  TWSD = 255-OCR2BL; break;
				case 4:  TWSD = 255-OCR0B; break;
				case 5:  TWSD = ADCH; break;
				case 6:  TWSD = 0xAA; break;
				default: TWSD = 0;
				}
				TWSCRB = _BV(TWCMD1) | _BV(TWCMD0); //Send ACK
				address++;
			}
			else
			{
				operation = TWSD; //Master sending data.
				TWSCRB = _BV(TWCMD1) | _BV(TWCMD0); //Send ACK

				if( state == 0 )
				{
					address = operation;
					state = 1;
				}
				else
				{
					//Writing...
					switch( address )
					{
					default:
						//unused
						break;
					case 1: //PWM 0
						OCR1BL = 255-operation;
						timeout = TIMEOUT;
						break;
					case 2: //PWM 1
						OCR1AL = 255-operation;
						timeout = TIMEOUT;
						break;
					case 3: //PWM 2
						OCR2BL = 255-operation;
						timeout = TIMEOUT;
						break;
					case 4: //PWM 3
						OCR0B = 255-operation;
						timeout = TIMEOUT;
						break;
					}
					address++;
				}
			}
		}
		if( TWSSRA & _BV( TWASIF ) )
		{
			if( TWSSRA & _BV(TWAS) )
			{
				//We were addressed...
				TWSCRB = _BV(TWCMD1) | _BV(TWCMD0); //Send ACK
				state = 0;
			}
			else
			{
				//We got a stop or other operation.
			   	TWSCRB = _BV(TWAA) | _BV(TWCMD1) | _BV(TWCMD0); //Send NAK
				state = 0;
			}
		}
	}

	return 0;
} 
