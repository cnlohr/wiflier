#ifndef _MYSTUFF_H
#define _MYSTUFF_H

#include <mem.h>
#include <c_types.h>
#include <user_interface.h>
#include <ets_sys.h>
#include <espconn.h>

extern char generic_print_buffer[384];

extern const char * enctypes[6];// = { "open", "wep", "wpa", "wpa2", "wpa_wpa2", "max" };

#define printf( ... ) ets_sprintf( generic_print_buffer, __VA_ARGS__ );  uart0_sendStr( generic_print_buffer );

int32 my_atoi( const char * in );
void Uint32To10Str( char * out, uint32 dat );

//For holding TX packet buffers
extern char generic_buffer[1500];
extern char * generic_ptr;
int8_t TCPCanSend( struct espconn * conn, int size );
int8_t TCPDoneSend( struct espconn * conn );
void EndTCPWrite( struct espconn * conn );

#define PushByte( c ) { *(generic_ptr++) = c; }

void PushString( const char * buffer );
void PushBlob( const uint8 * buffer, int len );
#define START_PACK {generic_ptr=generic_buffer;}
#define PACK_LENGTH (generic_ptr-&generic_buffer[0]}


int ColonsToInts( const char * str, int32_t * vals, int max_quantity );

#endif
