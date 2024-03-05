#include "screenshot.h"

#include <cerrno>
#include <cstdio>
#include <iostream>

#include "libtwice/nds/defs.h"
#include "libtwice/types.h"

#ifdef TWICE_HAVE_PNG
#  include <png.h>
#endif

namespace twice {

static int
write_png(void *fb, FILE *fp)
{
#ifdef TWICE_HAVE_PNG
	png_structp png_ptr = png_create_write_struct(
			PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		return 1;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return 1;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return 1;
	}

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, NDS_FB_W, NDS_FB_H, 8,
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_bytepp row_pointers = (png_bytepp)png_malloc(
			png_ptr, NDS_FB_H * sizeof *row_pointers);
	for (u32 i = 0; i < NDS_FB_H; i++) {
		row_pointers[i] = (png_bytep)fb + i * NDS_FB_W * 4;
	}

	png_write_info(png_ptr, info_ptr);
	png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return 0;
#else
	return 1;
#endif
}

int
write_nds_bitmap_to_png(void *fb, const std::string& filename)
{
	FILE *fp = fopen(filename.c_str(), "wxb");
	if (!fp) {
		return errno;
	}

	if (write_png(fb, fp)) {
		fclose(fp);
		return 1;
	}

	fclose(fp);
	return 0;
}

} // namespace twice
