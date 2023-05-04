#include "libtwice/arm/arm9.h"
#include "libtwice/mem/bus.h"
#include "libtwice/util.h"

using namespace twice;

void
Arm9::jump(u32 addr)
{
	if (in_thumb()) {
		pc() = addr + 2;
		pipeline[0] = fetch16(addr);
		pipeline[1] = fetch16(addr + 2);
	} else {
		pc() = addr + 4;
		pipeline[0] = fetch32(addr);
		pipeline[1] = fetch32(addr + 4);
	}
}

template <typename T>
T
Arm9::fetch(u32 addr)
{
	if (read_itcm && addr < itcm_virtual_size) {
		return readarr<T>(itcm, addr & itcm_mask);
	}

	return bus9_read<T>(nds, addr);
}

template <typename T>
T
Arm9::load(u32 addr)
{
	if (read_itcm && addr < itcm_virtual_size) {
		return readarr<T>(itcm, addr & itcm_mask);
	}

	if (read_dtcm && dtcm_base <= addr &&
			addr < dtcm_base + dtcm_virtual_size) {
		return readarr<T>(dtcm, addr & dtcm_mask);
	}

	return bus9_read<T>(nds, addr);
}

template <typename T>
void
Arm9::store(u32 addr, T value)
{
	if (read_itcm && addr < itcm_virtual_size) {
		writearr<T>(itcm, addr & itcm_mask, value);
	}

	if (read_dtcm && dtcm_base <= addr &&
			addr < dtcm_base + dtcm_virtual_size) {
		writearr<T>(dtcm, addr & dtcm_mask, value);
	}

	bus9_write<T>(nds, addr, value);
}

u32
Arm9::fetch32(u32 addr)
{
	return fetch<u32>(addr);
}

u16
Arm9::fetch16(u32 addr)
{
	return fetch<u16>(addr);
}

u32
Arm9::load32(u32 addr)
{
	return load<u32>(addr);
}

u16
Arm9::load16(u32 addr)
{
	return load<u16>(addr);
}

u8
Arm9::load8(u32 addr)
{
	return load<u8>(addr);
}

void
Arm9::store32(u32 addr, u32 value)
{
	store<u32>(addr, value);
}

void
Arm9::store16(u32 addr, u16 value)
{
	store<u16>(addr, value);
}

void
Arm9::store8(u32 addr, u8 value)
{
	store<u8>(addr, value);
}

void
Arm9::cp15_write(u32 reg, u32 value)
{
	(void)reg;
	(void)value;
}
