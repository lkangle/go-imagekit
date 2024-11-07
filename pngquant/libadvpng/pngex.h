/*
 * This file is part of the Advance project.
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

#ifndef __PNGEX_H
#define __PNGEX_H

#include "adv-compress.h"
#include "data.h"

#include "lib/png.h"
#include "lib/error.h"

#ifdef __cplusplus
extern "C" {
#endif
    //void png_compress(
    //        shrink_t level,
    //        data_ptr &out_ptr, unsigned &out_size,
    //        const unsigned char *img_ptr, unsigned img_scanline, unsigned img_pixel,
    //        unsigned x, unsigned y, unsigned dx, unsigned dy
    //);
    int png_compress(
            shrink_t level,
            data_ptr* out_ptr, unsigned *out_size,
            const unsigned char *img_ptr, unsigned img_scanline, unsigned img_pixel,
            unsigned x, unsigned y, unsigned dx, unsigned dy
    );

    int png_write(
            adv_fz *f,
            unsigned pix_width, unsigned pix_height, unsigned pix_pixel,
            unsigned char *pix_ptr, unsigned pix_scanline,
            unsigned char *pal_ptr, unsigned pal_size,
            unsigned char *rns_ptr, unsigned rns_size,
            shrink_t level
    );
#ifdef __cplusplus
}
#endif
#endif

