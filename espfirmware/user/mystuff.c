#include "mystuff.h"

const char * enctypes[6] = { "open", "wep", "wpa", "wpa2", "wpa_wpa2", "max" };

char generic_print_buffer[384];
char generic_buffer[1500];
char * generic_ptr;

int32 ICACHE_FLASH_ATTR my_atoi( const char * in )
{
	int positive = 1; //1 if negative.
	int hit = 0;
	int val = 0;
	while( *in && hit < 11 	)
	{
		if( *in == '-' )
		{
			if( positive == -1 ) return val*positive;
			positive = -1;
		} else if( *in >= '0' && *in <= '9' )
		{
			val *= 10;
			val += *in - '0';
			hit++;
		} else if (!hit && ( *in == ' ' || *in == '\t' ) )
		{
			//okay
		} else
		{
			//bad.
			return val*positive;
		}
		in++;
	}
	return val*positive;
}

void Uint32To10Str( char * out, uint32 dat )
{
	int tens = 1000000000;
	int val;
	int place = 0;

	while( tens > 1 )
	{
		if( dat/tens ) break;
		tens/=10;
	}

	while( tens )
	{
		val = dat/tens;
		dat -= val*tens;
		tens /= 10;
		out[place++] = val + '0';
	}

	out[place] = 0;
}

void EndTCPWrite( struct 	espconn * conn )
{
	if(generic_ptr!=generic_buffer)
		espconn_sent(conn,generic_buffer,generic_ptr-generic_buffer);
}


void PushString( const char * buffer )
{
	char c;
	while( c = *(buffer++) )
		PushByte( c );
}

void PushBlob( const uint8 * buffer, int len )
{
	int i;
	for( i = 0; i < len; i++ )
		PushByte( buffer[i] );
}


int8_t TCPCanSend( struct espconn * conn, int size )
{
	struct espconn_packet infoarg;
	sint8 r = espconn_get_packet_info(conn, &infoarg);

	if( infoarg.snd_buf_size >= size && infoarg.snd_queuelen > 0 )
		return 1;
	else
		return 0;
}

int8_t TCPDoneSend( struct espconn * conn )
{
	return conn->state == ESPCONN_CONNECT;
}

const char * my_strchr( const char * st, char c )
{
	while( *st && *st != c ) st++;
	if( !*st ) return 0;
	return st;
}

int ColonsToInts( const char * str, int32_t * vals, int max_quantity )
{
	int i;
	for( i = 0; i < max_quantity; i++ )
	{
		const char * colon = my_strchr( str, ':' );
		vals[i] = my_atoi( str );
		if( !colon ) break;
		str = colon+1;
	}
	return i+1;
}



