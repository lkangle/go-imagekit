#include "stdlib.h"
#include "stdint.h"

/**
 * 二次压缩
 *
 * 不同等级对应不同压缩器(1-libdeflate 2-7z 3-zopfli) 压缩等级越高越耗时
 *
 * @param input_buffer 要压缩的png图片数据，必须是png格式
 * @param input_size   输入图片大小
 * @param out_image
 * @param out_size
 * @param iter         压缩迭代次数
 * @param level        压缩等级 (1-3)
 * @return 无错误为0
 */
int rezip_png(uint8_t *input_buffer, size_t input_size, uint8_t **out_image, size_t *out_size,
              int iter, int level);

const char * get_version();
