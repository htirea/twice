CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
OBJCOPY=arm-none-eabi-objcopy
CFLAGS=-Os -Wall -Wextra -marm -mcpu=arm7tdmi
ASFLAGS=-mcpu=arm7tdmi
LDFLAGS=-nostdlib -lgcc

target = dldi_driver
OBJECTS = header.o driver.o

$(target).bin: $(OBJECTS) Makefile
	$(CC) -Tdldi_driver.ld -o $(target).elf $(OBJECTS) $(LDFLAGS)
	$(OBJCOPY) -O binary $(target).elf $(target).bin

.PHONY: clean
clean:
	rm -f $(target).bin $(target).elf $(OBJECTS)
