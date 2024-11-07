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

#include "portable.h"
#include "adv-compress.h"
#include "data.h"

bool compress_rfc1950_libdeflate(const unsigned char* in_data, unsigned in_size,
                                 unsigned char* out_data, unsigned* out_size, int compression_level)
{
	struct libdeflate_compressor* compressor;

	compressor = libdeflate_alloc_compressor(compression_level);

	size_t zc_size = libdeflate_zlib_compress(compressor, in_data, in_size, out_data, *out_size);

	libdeflate_free_compressor(compressor);

	if (!zc_size)
		return false;

    *out_size = zc_size;

	return true;
}

bool compress_zlib(shrink_t level, unsigned char* out_data, unsigned* out_size,
                   const unsigned char* in_data, unsigned in_size)
{
    // 最高等级
	if (level.level == shrink_insane) {
		ZopfliOptions opt_zopfli;
		unsigned char* data;
		size_t size;

		ZopfliInitOptions(&opt_zopfli);
		opt_zopfli.numiterations = level.iter > 5 ? level.iter : 5;

		size = 0;
		data = NULL;

        if (ZopfliCompress(&opt_zopfli, ZOPFLI_FORMAT_ZLIB, in_data, in_size, &data, &size) != 0){
            return false;
        }

		if (size < *out_size) {
			memcpy(out_data, data, size);
			*out_size = size;
		}

		free(data);
        return true;
	}

    int compression_level = 12;
    unsigned char* data;
    unsigned size;

    size = *out_size;
    data = data_alloc(size);

    if (compress_rfc1950_libdeflate(in_data, in_size, data, &size, compression_level)) {
        memcpy(out_data, data, size);
        *out_size = size;
    }

    data_free(data);
    return true;
}

unsigned oversize_deflate(unsigned size)
{
	return size + size / 10 + 12;
}

unsigned oversize_zlib(unsigned size)
{
	return oversize_deflate(size) + 10;
}

