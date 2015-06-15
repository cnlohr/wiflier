#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "espconn.h"
#include "i2c.h"
#include "control.h"
#include "mystuff.h"
#include "ip_addr.h"
#include "http.h"
#include "spi_flash.h"
#include "esp8266_rom.h"

#define MAX_FRAME 2000

#define procTaskPrio        0
#define procTaskQueueLen    1

static volatile os_timer_t some_timer;
static struct espconn *pUdpServer;
static struct espconn *pHTTPServer;



//Tasks that happen all the time.

int jr;
os_event_t    procTaskQueue[procTaskQueueLen];

volatile int tick_flag = 0;

static void ICACHE_FLASH_ATTR
procTask(os_event_t *events)
{
	static int printed_ip;
	system_os_post(procTaskPrio, 0, 0 );
	jr++;

	HTTPTick(0);

	//TRICKY! If we use the timer to initiate this, connecting to people's networks
	//won't work!  I don't know why, so I do it here.  this does mean sometimes it'll
	//pause, though.
	if( tick_flag )
	{
		tick_flag = 0;
		controltimer( );
		HTTPTick(1);
	}

	if( events->sig == 0 && events->par == 0 )
	{
		//Idle Event.
		struct station_config wcfg;
		char stret[256];
		char *stt = &stret[0];
		struct ip_info ipi;

		int stat = wifi_station_get_connect_status();

//		printf( "STAT: %d\n", stat );

		if( stat == STATION_WRONG_PASSWORD || stat == STATION_NO_AP_FOUND || stat == STATION_CONNECT_FAIL )
		{
			wifi_set_opmode_current( 2 );
			stt += ets_sprintf( stt, "Connection failed: %d\n", stat );
			uart0_sendStr(stret);
		}

		if( stat == STATION_GOT_IP && !printed_ip )
		{
			wifi_station_get_config( &wcfg );
			wifi_get_ip_info(0, &ipi);
			stt += ets_sprintf( stt, "STAT: %d\n", stat );
			stt += ets_sprintf( stt, "IP: %d.%d.%d.%d\n", (ipi.ip.addr>>0)&0xff,(ipi.ip.addr>>8)&0xff,(ipi.ip.addr>>16)&0xff,(ipi.ip.addr>>24)&0xff );
			stt += ets_sprintf( stt, "NM: %d.%d.%d.%d\n", (ipi.netmask.addr>>0)&0xff,(ipi.netmask.addr>>8)&0xff,(ipi.netmask.addr>>16)&0xff,(ipi.netmask.addr>>24)&0xff );
			stt += ets_sprintf( stt, "GW: %d.%d.%d.%d\n", (ipi.gw.addr>>0)&0xff,(ipi.gw.addr>>8)&0xff,(ipi.gw.addr>>16)&0xff,(ipi.gw.addr>>24)&0xff );
			stt += ets_sprintf( stt, "WCFG: /%s/%s/\n", wcfg.ssid, wcfg.password );
			uart0_sendStr(stret);
			printed_ip = 1;
		}
	}
}




void ICACHE_FLASH_ATTR at_recvTask()
{
	//Called from UART.	
}

void ICACHE_FLASH_ATTR MyTimer(void *arg)
{
	tick_flag = 1;
}

void user_rf_pre_init(void)
{
	//nothing.
}

char * strcat( char * dest, char * src )
{
	return strcat(dest, src );
}

void ICACHE_FLASH_ATTR user_init(void)
{
	int i;
restart:

	uart_init(BIT_RATE_115200, BIT_RATE_115200);

 	gpio_init();

	//Remap MTD to GPIO
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12); 
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14); 

	//~60k resistor.  On a small PCB, with 3 I2C clients, it takes ~ 1uS to get to 2v
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDI_U);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTMS_U); 

	gpio_output_set( 0x00000000, 0x00000000, 0x00000000, 0xffffffff );

	ConfigI2C();

	ControlInit(); //Reads out configuration.

	int wifiMode = wifi_get_opmode();
	printf( "Wifi Mode: %d (", wifiMode );
	if( wifiMode == SOFTAP_MODE )
	{
		struct softap_config c;
		wifi_softap_get_config( &c );

		printf( "SoftAP)\n SSID: %s\n PASS: %s\n Channel: %d\n Auth Mode: %s\n", c.ssid, c.password, c.channel, enctypes[c.authmode] );
	}
	else if( wifiMode == STATION_MODE )
	{
		struct station_config c;
		wifi_station_get_config( &c );
		wifi_station_dhcpc_start();
		printf( "Station)\n SSID: %s\n PASS: %s\n BSSID SET: %d\n", c.ssid, c.password, c.bssid_set );
	}
	else
	{
		printf( "%d)\n", wifiMode );
	}

//Nope! Read configuration from flash.
//	wifi_set_opmode( 2 ); //We broadcast our ESSID, wait for peopel to join.

    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = 7777;
	espconn_regist_recvcb(pUdpServer, issue_command);

	if( espconn_create( pUdpServer ) )
	{
		uart0_sendStr( "UDP FAULT\r\n" );
		goto restart;
	}

	pHTTPServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pHTTPServer, 0, sizeof( struct espconn ) );
	espconn_create( pHTTPServer );
	pHTTPServer->type = ESPCONN_TCP;
    pHTTPServer->state = ESPCONN_NONE;
	pHTTPServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	pHTTPServer->proto.tcp->local_port = 80;
    espconn_regist_connectcb(pHTTPServer, httpserver_connectcb);
    espconn_accept(pHTTPServer);
    espconn_regist_time(pHTTPServer, 15, 0); //timeout


//	ets_wdt_disable();

	int i2ctries = 0;
printf( "Starting I2C\n" );
retryi2c:
	ConfigI2C();

	if( i2ctries++ > 100 )
	{
		goto skip_i2c;
	}

	if( SetupAVRTool() )
	{
		uart0_sendStr("Retry AVRTool\r\n");
		goto retryi2c;		
	}

	if( SetupBMP085() )
	{
		uart0_sendStr("Retry BMP\r\n");
		goto retryi2c;
	}

	if( SetupLSM() )
	{
		uart0_sendStr("Retry LSM\r\n");
		goto retryi2c;
	}

skip_i2c:

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	uart0_sendStr("\r\nCustom Server\r\n");
	uart0_sendStr("NewTest5\r\n");

	ControlInit();
	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)MyTimer, NULL);
	os_timer_arm(&some_timer, 10, 1); // was 6, 166.66 Hz update rate... let's try 100hz


	system_os_post(procTaskPrio, 0, 0 );
}


