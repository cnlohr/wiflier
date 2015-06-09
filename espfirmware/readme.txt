
This program needs to be compiled using the esp_iot_sdk_v1.1.0.

Memory layout:

Traditional memory map:
Address 	Size 	Name 	Description
00000h		40k 	0x00000.bin (Program that resides in IRAM) Bootloader copies this 40k.
0A000h		204k    (unused?)
3D000h      4k      our device configuration
3E000h		8k 	    master_device_key.bin??? (I don't know what this is used for)
40000h		240k 	app.v6.irom0text.bin 
7C000h		8k 	    esp_init_data_default.bin 	Default configuration
7E000h		8k 	    blank.bin 	Filled with FFh. May be WiFi configuration
80000h		512k	Scratchpad. (Used for remote program flashing)
100000h		1M+		HTTP data, 1M on W25Q16


TODO:

*Is it possible to monitor all the sensors?
*Try one-shotting the sensors.
*Add WebGL
*Add WebSockets
*Try turning off pullups on the AVR
*Timeout the AVR.
*Separate the control.c back out.
*Enable softap-mode-scanning

Project moved to github. https://github.com/cnlohr/wiflier
