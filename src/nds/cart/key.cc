#include "nds/cart/key.h"

#include "common/util.h"
#include "nds/cart/cart.h"

namespace twice {

void
encrypt_secure_area(cartridge *cart)
{
	if (cart->size < 0x8000)
		return;

	u64 id = readarr<u64>(cart->data, 0x4000);
	if (id != 0xE7FFDEFFE7FFDEFF)
		return;

	cart_init_keycode(cart, cart->gamecode, 3, 8);
	std::memcpy(cart->data + 0x4000, "encryObj", 8);
	for (u32 i = 0; i < 0x800; i += 8) {
		cart_encrypt64(cart->data + 0x4000 + i, cart->keybuf);
	}

	cart_init_keycode(cart, cart->gamecode, 2, 8);
	cart_encrypt64(cart->data + 0x4000, cart->keybuf);
}

void
cart_init_keycode(cartridge *cart, u32 gamecode, int level, u32 modulo)
{
	std::memcpy(cart->keybuf, cart->keybuf_s, sizeof(cart->keybuf));

	writearr<u32>(cart->keycode, 0, gamecode);
	writearr<u32>(cart->keycode, 4, gamecode / 2);
	writearr<u32>(cart->keycode, 8, gamecode * 2);
	if (level >= 1)
		cart_apply_keycode(cart, modulo);
	if (level >= 2)
		cart_apply_keycode(cart, modulo);
	writearr<u32>(cart->keycode, 4, readarr<u32>(cart->keycode, 4) * 2);
	writearr<u32>(cart->keycode, 8, readarr<u32>(cart->keycode, 8) / 2);
	if (level >= 3)
		cart_apply_keycode(cart, modulo);
}

void
cart_apply_keycode(cartridge *cart, u32 modulo)
{
	cart_encrypt64(cart->keycode + 4, cart->keybuf);
	cart_encrypt64(cart->keycode, cart->keybuf);
	for (u32 i = 0; i <= 0x11; i++) {
		cart->keybuf[i] ^= byteswap32(
				readarr<u32>(cart->keycode, i * 4 % modulo));
	}

	u8 scratch[8]{};
	for (u32 i = 0; i <= 0x410; i += 2) {
		cart_encrypt64(&scratch[0], cart->keybuf);
		cart->keybuf[i] = readarr<u32>(scratch, 4);
		cart->keybuf[i + 1] = readarr<u32>(scratch, 0);
	}
}

void
cart_encrypt64(u8 *p, u32 *keybuf)
{
	u32 y = readarr<u32>(p, 0);
	u32 x = readarr<u32>(p, 4);

	for (u32 i = 0; i <= 0xF; i++) {
		u32 z = keybuf[i] ^ x;
		x = keybuf[0x012 + (z >> 24 & 0xFF)];
		x += keybuf[0x112 + (z >> 16 & 0xFF)];
		x ^= keybuf[0x212 + (z >> 8 & 0xFF)];
		x += keybuf[0x312 + (z & 0xFF)];
		x ^= y;
		y = z;
	}

	writearr<u32>(p, 0, x ^ keybuf[0x10]);
	writearr<u32>(p, 4, y ^ keybuf[0x11]);
}

void
cart_decrypt64(u8 *p, u32 *keybuf)
{
	u32 y = readarr<u32>(p, 0);
	u32 x = readarr<u32>(p, 4);

	for (u32 i = 0x11; i >= 0x2; i--) {
		u32 z = keybuf[i] ^ x;
		x = keybuf[0x012 + (z >> 24 & 0xFF)];
		x += keybuf[0x112 + (z >> 16 & 0xFF)];
		x ^= keybuf[0x212 + (z >> 8 & 0xFF)];
		x += keybuf[0x312 + (z & 0xFF)];
		x ^= y;
		y = z;
	}

	writearr<u32>(p, 0, x ^ keybuf[0x1]);
	writearr<u32>(p, 4, y ^ keybuf[0x0]);
}

} // namespace twice
