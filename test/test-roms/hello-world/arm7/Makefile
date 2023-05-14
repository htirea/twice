arm7.bin: entry.o
	$(LD) -Tarm7.ld -o arm7.out entry.o
	$(OBJCOPY) -O binary arm7.out arm7.bin

entry.o: entry.asm
	$(AS) -mcpu=arm7tdmi -o entry.o entry.asm

.PHONY: clean
clean:
	rm -f arm7.bin arm7.out entry.o
