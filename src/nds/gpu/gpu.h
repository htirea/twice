#ifndef TWICE_GPU_H
#define TWICE_GPU_H

#include "common/types.h"
#include "common/util.h"

namespace twice {

struct NDS;

struct Gpu2D {
	Gpu2D(NDS *nds, int engineid);

	u32 dispcnt{};
	u16 bg_cnt[4]{};
	u16 bg_hofs[4]{};
	u16 bg_vofs[4]{};
	u16 bg_pa[2]{};
	u16 bg_pb[2]{};
	u16 bg_pc[2]{};
	u16 bg_pd[2]{};
	s32 bg_ref_x[2]{};
	s32 bg_ref_y[2]{};
	u32 bg_ref_x_latch[2]{};
	u32 bg_ref_y_latch[2]{};
	bool bg_ref_x_reload[2]{};
	bool bg_ref_y_reload[2]{};
	u16 win_h[2]{};
	u16 win_v[2]{};
	u16 winin{};
	u16 winout{};
	u16 mosaic{};
	u16 bldcnt{};
	u16 bldalpha{};
	u16 bldy{};

	u32 *fb{};

	struct Pixel {
		u32 color{};
		u8 priority{};
		/* TODO: use bitfields to save space */
	};

	Pixel bg_buffer_top[NDS_SCREEN_W]{};
	Pixel bg_buffer_bottom[NDS_SCREEN_W]{};

	bool enabled{};
	NDS *nds{};
	int engineid{};

	u32 read32(u8 offset);
	u16 read16(u8 offset);
	void write32(u8 offset, u32 value);
	void write16(u8 offset, u16 value);

	void draw_scanline(u16 scanline);
	void graphics_display_scanline();
	void set_backdrop();
	void render_text_bg(int bg);
	u64 fetch_char_row(u16 se, u32 char_base, u32 bg_y, bool color_256);
	void draw_bg_pixel(u32 fb_x, u16 color, u8 priority);
	void render_affine_bg(int bg);
	void render_extended_bg(int bg);
	void render_extended_text_bg(int bg);
	void render_extended_bitmap_bg(int bg, bool direct_color);
	void vram_display_scanline();

	u16 get_screen_entry(u32 screen, u32 base, u32 x, u32 y);
	u8 get_screen_entry_affine(u32 base, u32 bg_w, u32 x, u32 y);
	u16 get_screen_entry_extended_text(u32 base, u32 bg_w, u32 x, u32 y);
	u64 get_char_row_256(u32 base, u32 char_name, u32 y);
	u32 get_char_row_16(u32 base, u32 char_name, u32 y);
	u8 get_color_num_256(u32 base, u32 char_name, u32 x, u32 y);
	u16 get_palette_color_256(u32 color_num);
	u16 get_palette_color_16(u32 palette_num, u32 color_num);
	template <typename T> T read_bg_data(u32 base, u32 w, u32 x, u32 y);
};

void gpu_on_scanline_start(NDS *nds);

} // namespace twice

#endif
