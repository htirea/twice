arm9.bin: arm9.o
	$(LD) -Tarm9.ld -o arm9.out arm9.o
	$(OBJCOPY) -O binary arm9.out arm9.bin

arm9.o: arm9.c Makefile util.h font8x8_basic.h
	$(CC) -c -mcpu=arm946e-s -o arm9.o arm9.c

.PHONY: clean
clean:
	rm -f arm9.bin arm9.out arm9.o
