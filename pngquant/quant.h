#ifndef PNGQUANT_QUANT_H
#define PNGQUANT_QUANT_H
#include <stdlib.h>

/**
 * 量化并二次压缩png图片
 * @param input
 * @param input_size
 * @param out
 * @param outsize
 * @param min_quality
 * @param max_quality
 * @param floyd 抖动级别 0-1
 * @param speed 速度 默认3
 * @param rzlevel 压缩等级 1，2
 * @return
 */
int PNGCompress(unsigned char *input, size_t input_size,
                unsigned char **out, size_t *outsize, int *width, int *height,
                int min_quality, int max_quality, float floyd, int speed, int rzlevel);

int PNGDecode(unsigned char *input, size_t input_size, unsigned char **rgba, int *width, int *height);

#endif // PNGQUANT_QUANT_H
