/*
 * This file is part of the AdvanceSCAN project.
 *
 * Copyright (C) 2002 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "portable.h"

#include "lib/endianrw.h"

#include "pngex.h"

int png_compress(
        shrink_t level,
        data_ptr* out_ptr, unsigned *out_size,
        const unsigned char *img_ptr, unsigned img_scanline, unsigned img_pixel,
        unsigned x, unsigned y, unsigned dx, unsigned dy
){
	data_ptr fil_ptr;
	unsigned fil_size;
	unsigned fil_scanline;
	data_ptr z_ptr;
	unsigned z_size;
	unsigned i;
	unsigned char* p0;

	fil_scanline = dx * img_pixel + 1;
	fil_size = dy * fil_scanline;
	z_size = oversize_zlib(fil_size);

	fil_ptr = data_alloc(fil_size);
	z_ptr = data_alloc(z_size);

	p0 = fil_ptr;

	for(i=0;i<dy;++i) {
		const unsigned char* p1 = &img_ptr[x * img_pixel + (i+y) * img_scanline];
		*p0++ = 0;
		memcpy(p0, p1, dx * img_pixel);
		p0 += dx * img_pixel;
	}

    if (p0 != fil_ptr + fil_size) {
        return -1;
    }

	if (!compress_zlib(level, z_ptr, &z_size, fil_ptr, fil_size)) {
        return -1;
	}

	*out_ptr = z_ptr;
	*out_size = z_size;
    return 0;
}

int png_write(
        adv_fz *f,
        unsigned pix_width, unsigned pix_height, unsigned pix_pixel,
        unsigned char *pix_ptr, unsigned pix_scanline,
        unsigned char *pal_ptr, unsigned pal_size,
        unsigned char *rns_ptr, unsigned rns_size,
        shrink_t level
){
	unsigned char ihdr[13];
	data_ptr z_ptr = NULL;
	unsigned z_size;

	if (adv_png_write_signature(f, 0) != 0) {
        return -1;
	}

	be_uint32_write(ihdr + 0, pix_width);
	be_uint32_write(ihdr + 4, pix_height);
	ihdr[8] = 8; /* bit depth */
	if (pix_pixel == 1)
		ihdr[9] = 3; /* color type */
	else if (pix_pixel == 3)
		ihdr[9] = 2; /* color type */
	else if (pix_pixel == 4)
		ihdr[9] = 6; /* color type */
	else
        return -1;

	ihdr[10] = 0; /* compression */
	ihdr[11] = 0; /* filter */
	ihdr[12] = 0; /* interlace */

	if (adv_png_write_chunk(f, ADV_PNG_CN_IHDR, ihdr, sizeof(ihdr), 0) != 0) {
        return -1;
	}

	if (pal_size) {
		if (adv_png_write_chunk(f, ADV_PNG_CN_PLTE, pal_ptr, pal_size, 0) != 0) {
            return -1;
		}
	}

	if (rns_size) {
		if (adv_png_write_chunk(f, ADV_PNG_CN_tRNS, rns_ptr, rns_size, 0) != 0) {
            return -1;
		}
	}

    if (png_compress(level, &z_ptr, &z_size, pix_ptr, pix_scanline, pix_pixel, 0, 0, pix_width, pix_height) != 0) {
        return -1;
    }

	if (adv_png_write_chunk(f, ADV_PNG_CN_IDAT, z_ptr, z_size, 0) != 0) {
        return -1;
	}

	if (adv_png_write_chunk(f, ADV_PNG_CN_IEND, 0, 0, 0) != 0) {
        return -1;
	}
    return 0;
}
