#!/bin/bash

set -e

AS=arm-none-eabi-as
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

$AS -march=armv5te -o arm9.o arm9.asm
$AS -march=armv4t -o arm7.o arm7.asm
$LD -Ttext 0x2000000 -e _arm9_start -o arm9.out arm9.o
$LD -Ttext 0x3800000 -e _arm7_start -o arm7.out arm7.o
$OBJCOPY -O binary arm9.out arm9.bin
$OBJCOPY -O binary arm7.out arm7.bin

ndstool -c arm9.nds -r9 0x2000000 -e9 0x2000000 -r7 0x3800000 -e7 0x3800000 -9 arm9.bin -7 arm7.bin

rm -f arm9.o arm7.o arm9.out arm7.out arm9.bin arm7.bin
