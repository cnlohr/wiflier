# wiflier
ESP8266-based Quadcopter Brain

WARNING: This project is still in its early stages!  It doesn't fly yet!

Do you think quadcopters are neat?  Eh, not really for me... BUT I THINK ESP8266's ARE AWESOME!  So, I slapped an LSM9DS1 IMU, a BMP085 barometer and an AVR co-processor with 4 PWM output drivers to a PCB small enough to fit in a hubsan X4 quadcopter.


## Building (Section under construction)

This code is separated into two pieces, the ESP firmware and the AVR firmware.

The ESP firmware requires the esp_iot_sdk_v1.1.0.

The AVR firmware requires avr-libc-1.8.0 and avr-gcc-4.8.1 at a minimum.

## Memory layout:

| Address | Size  | Name / Description        |
| ------- |:-----:| ------------------------- |
| 00000h  | 40k   | 0x00000.bin, IRAM Code    | 
| 0A000h  | 204k  | (unused?)                 |
| 3D000h  | 4k    | our device configuration  |
| 3E000h  | 8k    | May be used by ESP SDK.   |
| 40000h  | 240k  | 0x40000.bin, Cached code. |
| 7C000h  | 8k    | May be used by ESP SDK.   |
| 7E000h  | 8k    | May be WiFi configuration |
| 80000h  | 512k  | Scratchpad (Temp only!)   |
| 100000h | 1M+   | HTTP data, 1M on W25Q16   |


## TODO:

* Try one-shotting the sensors (currently they're in free-run)
* Add WebGL?
* Add WebSockets?
* Explain how commands work.
* Try turning off pullups on the AVR
* Separate the control.c into different parts.


## Final Notes
Project moved to github. https://github.com/cnlohr/wiflier
