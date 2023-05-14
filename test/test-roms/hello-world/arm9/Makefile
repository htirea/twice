arm9.bin: main.o entry.o
	$(LD) -Tarm9.ld -o arm9.out main.o entry.o
	$(OBJCOPY) -O binary arm9.out arm9.bin

main.o: main.c Makefile util.h font8x8_basic.h
	$(CC) $(CFLAGS) -c -mcpu=arm946e-s -o main.o main.c

entry.o: entry.asm
	$(AS) -mcpu=arm946e-s -o entry.o entry.asm

.PHONY: clean
clean:
	rm -f arm9.bin arm9.out main.o entry.o
