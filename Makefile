TARGET = SPI-04-FatFs

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

# -I puts a folder in the "include" path for the compiler
CFLAGS = -mcpu=cortex-m4 -mthumb -O0 -g -Wall -Iff16/source -Iheaders 
LDFLAGS = -T stm32f446.ld -nostartfiles

SRCS = $(TARGET).c headers/spi.c headers/sd_card.c headers/uart.c startup.s \
ff16/source/ff.c ff16/source/diskio.c \
ff16/source/ffunicode.c 

# commands inside receipes are required to be indented
$(TARGET).elf: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

clean:
	rm -f *.elf *.bin
