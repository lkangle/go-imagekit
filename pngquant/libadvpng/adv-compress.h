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

#ifndef __COMPRESS_H
#define __COMPRESS_H
//#include "7z/7z.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libdeflate/libdeflate.h"
#include "zopfli/zopfli.h"

#define RETRY_FOR_SMALL_FILES 65536 /**< Size for which we try multiple algorithms */

typedef enum SHRINK_LEVEL_T {
    // 以下有效
    shrink_normal,
    shrink_extra,
    shrink_insane
} shrink_level_t;

typedef struct SHRINK_T {
    shrink_level_t level;
    int iter;
} shrink_t;

typedef unsigned int bool;
#define true 1;
#define false 0;

unsigned oversize_deflate(unsigned size);
unsigned oversize_zlib(unsigned size);

bool compress_rfc1950_libdeflate(const unsigned char* in_data, unsigned in_size,
                                 unsigned char* out_data, unsigned* out_size, int compression_level);

bool compress_zlib(shrink_t level, unsigned char* out_data, unsigned* out_size,
                   const unsigned char* in_data, unsigned in_size);

#ifdef __cplusplus
};
#endif

#endif
