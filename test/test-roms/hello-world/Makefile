export CC=arm-none-eabi-gcc
export AS=arm-none-eabi-as
export OBJCOPY=arm-none-eabi-objcopy
export LD=arm-none-eabi-ld
export CFLAGS=-O2 -ffreestanding

target = hello-world.nds

all: $(target)

$(target): arm7 arm9
	ndstool -c $(target) -r9 0x2000000 -e9 0x2000000 -r7 0x3800000 -e7 0x3800000 -9 arm9/arm9.bin -7 arm7/arm7.bin

.PHONY: arm7
arm7:
	$(MAKE) -C arm7

.PHONY: arm9
arm9:
	$(MAKE) -C arm9

.PHONY: clean
clean:
	rm -f $(target)
	$(MAKE) -C arm7 clean
	$(MAKE) -C arm9 clean
