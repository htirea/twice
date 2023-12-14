#ifndef TWICE_CART_KEY_H
#define TWICE_CART_KEY_H

#include "common/types.h"

namespace twice {

struct cartridge;

void encrypt_secure_area(cartridge *cart);
void cart_init_keycode(cartridge *cart, u32 gamecode, int level, u32 modulo);
void cart_apply_keycode(cartridge *cart, u32 modulo);
void cart_encrypt64(u8 *p, u32 *keybuf);
void cart_decrypt64(u8 *p, u32 *keybuf);

} // namespace twice

#endif
