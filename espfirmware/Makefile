FW_FILE_1:=0x00000.bin
FW_FILE_2:=0x40000.bin
all : image.elf $(FW_FILE_1) $(FW_FILE_2)

TARGET_OUT:=image.elf
OBJS:=driver/uart.o \
	user/mystuff.o \
	user/i2c.o \
	user/user_main.o \
	user/bmp085.o \
	user/control.o \
	user/http.o \
	user/mfs.o \
	user/lsm9ds1.o \
	user/flash_rewriter.o \
	user/http_custom.o \
	user/avrtool.o 

SRCS:=driver/uart.c \
	user/mystuff.c \
	user/i2c.c \
	user/user_main.c  \
	user/bmp085.c \
	user/control.c \
	user/http.c \
	user/mfs.c \
	user/lsm9ds1.c \
	user/flash_rewriter.c \
	user/http_custom.c \
	user/avrtool.c


GCC_FOLDER:=~/esp8266/esp-open-sdk/xtensa-lx106-elf
ESPTOOL_PY:=~/esp8266/esptool/esptool.py
FW_TOOL:=~/esp8266/other/esptool/esptool
SDK:=/home/cnlohr/esp8266/esp_iot_sdk_v1.1.0
PORT:=/dev/ttyUSB0

XTLIB:=$(SDK)/lib
XTGCCLIB:=$(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/4.8.2/libgcc.a
FOLDERPREFIX:=$(GCC_FOLDER)/bin
PREFIX:=$(FOLDERPREFIX)/xtensa-lx106-elf-
CC:=$(PREFIX)gcc

CFLAGS:=-mlongcalls -I$(SDK)/include -Imyclib -Iinclude -Iuser -Os -I$(SDK)/include/ -DICACHE_FLASH



LDFLAGS_CORE:=\
	-nostdlib \
	-Wl,--relax -Wl,-Map,image.map  -Wl,--gc-sections \
	-flto \
	-L$(XTLIB) \
	-L$(XTGCCLIB) \
	-lgcc \
	$(SDK)/lib/libmain.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libwpa.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/libphy.a \
	$(SDK)/lib/libpp.a \
	$(XTGCCLIB) \
	 \
	-T $(SDK)/ld/eagle.app.v6.ld \


LINKFLAGS:= \
	$(LDFLAGS_CORE) \
	-B$(XTLIB)

#image.elf : $(OBJS)
#	$(PREFIX)ld $^ $(LDFLAGS) -o $@

$(TARGET_OUT) : $(SRCS)
	$(PREFIX)gcc $(CFLAGS) $^   $(LINKFLAGS) -o $@
	$(PREFIX)objdump -S $(TARGET_OUT) > image.lst

$(FW_FILE_1): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

$(FW_FILE_2): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -es .irom0.text $@ -ec

burn : $(FW_FILE_1) $(FW_FILE_2)
	($(ESPTOOL_PY) --port $(PORT) write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)


netburn : image.elf $(FW_FILE_1) $(FW_FILE_2)
	web/execute_reflash 192.168.4.1 0x00000.bin 0x40000.bin

terminal :
	@stty -F $(PORT) 115200 -echo raw && cat $(PORT)

clean :
	rm -rf user/*.o driver/*.o $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)


