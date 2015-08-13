
all : test.bin burn test.lst
PART=attiny841
PROGPART=t841
IP?=192.168.4.1

#You need a relatively new AVR toolchain to compile this.  It needs support for the 441.
#AVRP=~/cpptest/avr-toolchain/avr-toolchain/bin/avr-
AVRP=avr-

#I have not measured the frequency.
CFLAGS=-g -Wall -Os -mmcu=$(PART) -DF_CPU=8000000UL
ASFLAGS:=$(CFLAGS)
CC=avr-gcc

test.bin : test.elf
	$(AVRP)objdump -S test.elf  >  test.lst
	$(AVRP)objcopy -j .text -j .data -O binary test.elf test.bin 

test.hex : test.elf
	$(AVRP)objcopy -j .text -j .data -O ihex test.elf test.hex

test.elf : test.c
	$(AVRP)gcc -I  -g $(CFLAGS)   -mmcu=$(PART) -Wl,-Map,test.map -o $@ test.c -L/usr/lib/binutils/avr/2.18

pushavr : pushavr.c ../web/md5.c
	gcc -o $@ $^

burn : test.bin pushavr
	./pushavr $(IP) test.bin

clean : 
	rm -f *~ high.txt low.txt test.bin test.map test.elf *.o test.hex pushavr test.lst
