#include <stdio.h>
#include <math.h>

//

int main()
{
	int i;
	printf( "const uint8_t ThrustCurve[] = {" );
	for( i = 0; i < 256; i++ )
	{
		if( (i % 16) == 0 )
		{
			printf( "\n\t" );
		}

		int pwm = pow( (float)i/(float)256, .6) * 236.0;

		if ( pwm < 0 )  pwm = 0;

		if( i == 0 )
			pwm = 0;
		else
		{
			pwm += 20; //Deadband
		}

		if( pwm > 255 ) pwm = 255;

		printf( "%3d, ", pwm );
	}
	printf( "};\n" );
}

