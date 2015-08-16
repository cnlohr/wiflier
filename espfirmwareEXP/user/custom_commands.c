#include <commonservices.h>
#include <mystuff.h>

static const char * key = "";
static int keylen = 0;

int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;

	switch( pusrdata[1] )
	{
	case 'C': case 'c': //Custom command test
	{
		buffend += ets_sprintf( buffend, "CC\n" );
		return buffend-buffer;
	}
	case 'A': case 'a': //AVR Commands
	{
		switch( pusrdata[2] )
		{
		case 'R': case 'r': 
		{
			buffend += ets_sprintf( buffend, "AR\n" );
			return buffend-buffer;
		}
		case 'F': case 'f': 
		{
			int i;
			char     md5h[48];
			char     md5hraw[48];
			MD5_CTX md5ctx;
			char * colon = (char *) ets_strstr( (char*)&pusrdata[3], "\t" );
			char * colon2 = (colon)?((char *)ets_strstr( (char*)(colon+1), "\t" )):0;

			if( !colon || !colon2 ) break;

			*colon = 0; colon++;
			*colon2 = 0; colon2++;


			uint32_t from = my_atoi( &pusrdata[3] );
			uint32_t len = my_atoi( colon );
			char * md5 = colon2;

			printf( "caffer %d %d %s\n", from, len, md5 );
			MD5Init( &md5ctx );
			if( keylen )
				MD5Update( &md5ctx, key, keylen );
			SafeMD5Update( &md5ctx, (uint8_t*)0x40200000 + from, len );
			MD5Final( md5hraw, &md5ctx );

			for( i = 0; i < 16; i++ )
			{
				ets_sprintf( md5h+i*2, "%02x", md5hraw[i] );
			}

			printf( "AVR Program MD5s: %s == %s\n", md5h, md5 );
			for( i = 0; i < 32; i++ )
			{
				if( md5[i] != md5h[i] )
				{
					break;
				}
			}

			if( i != 32 ) break;

			int r = ProgramAVRFlash( (uint8_t*)0x40200000+from, len );

			if( r )
			{
				buffend += ets_sprintf( buffend, "!CAF\t%d\r\n", r );
			}
			else
			{
				buffend += ets_sprintf( buffend, "CAF\r\n" );
			}

			return buffend-buffer;
		}
		default: break;
		}
		buffend += ets_sprintf( buffend, "!CA%c\t-99\r\n", pusrdata[2] );
		return buffend-buffer;
	}

	}
	return -1;
}
