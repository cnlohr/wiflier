
all : test.hex burn test.lst
PART=attiny441
PROGPART=t441

#You need a relatively new AVR toolchain to compile this.  It needs support for the 441.
AVRP=~/cpptest/avr-toolchain/avr-toolchain/bin/avr-

#I have not measured the frequency.
CFLAGS=-g -Wall -Os -mmcu=$(PART) -DF_CPU=8000000UL
ASFLAGS:=$(CFLAGS)
CC=avr-gcc

test.hex : test.elf
	$(AVRP)objcopy -j .text -j .data -O ihex test.elf test.hex 

test.elf : test.c
	$(AVRP)gcc -I  -g $(CFLAGS)   -mmcu=$(PART) -Wl,-Map,test.map -o $@ test.c -L/usr/lib/binutils/avr/2.18

test.lst : test.c
	$(AVRP)gcc -c -g -Wa,-a,-ad $(CFLAGS) test.c > $@

burn : test.hex
	sudo avrdude -c usbtiny -p $(PROGPART) -U flash:w:test.hex

clean : 
	rm -f *~ high.txt low.txt test.hex test.map test.elf *.o sendpack.S makesendpack
