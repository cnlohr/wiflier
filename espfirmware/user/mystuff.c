#include "mystuff.h"

char generic_print_buffer[384];
char generic_buffer[1500];
char * generic_ptr;

int32 ICACHE_FLASH_ATTR my_atoi( const char * in )
{
	int negative = 0; //1 if negative.
	int hit = 0;
	int val = 0;
	while( *in && hit < 11 	)
	{
		if( *in == '-' )
		{
			if( negative ) return val;
			negative = 1;
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
			return val;
		}
		in++;
	}
	return val;
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


