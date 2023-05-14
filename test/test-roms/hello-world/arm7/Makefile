arm7.bin: arm7.o
	$(LD) -Tarm7.ld -o arm7.out arm7.o
	$(OBJCOPY) -O binary arm7.out arm7.bin

arm7.o: arm7.c Makefile
	$(CC) -c -mcpu=arm7tdmi -o arm7.o arm7.c

.PHONY: clean
clean:
	rm -f arm7.bin arm7.out arm7.o
