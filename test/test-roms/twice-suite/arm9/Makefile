CFLAGS += -mcpu=arm946e-s
ASFLAGS = -mcpu=arm966e-s

OBJECTS =  main.o entry.o writeback.o

arm9.bin: main.o entry.o writeback.o
	$(LD) -Tarm9.ld -o arm9.out $(OBJECTS)
	$(OBJCOPY) -O binary arm9.out arm9.bin

main.o: util.h font8x8_basic.h

.PHONY: clean
clean:
	rm -f arm9.bin arm9.out $(OBJECTS)
