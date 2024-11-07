#include "portable.h"
#include "pngex.h"
#include "advpng.h"

enum rezip_error {
    OPTION_ERROR = 5001,
    ZIP_ERROR = 5002
};

// 压缩等级和迭代次数
shrink_t opt_level;
char* version_str = "v1.0 by likangle.";

int reduce_image(unsigned char** out_ptr, unsigned* out_scanline, unsigned char* pal_ptr, unsigned* pal_count, unsigned char* palrns_ptr, unsigned *palrns_count, unsigned width, unsigned height, unsigned char* img_ptr, unsigned img_scanline, const unsigned char* rns_ptr, unsigned rns_size)
{
    unsigned char col_ptr[256*3];
    unsigned col_count;
    unsigned i, j, k;
    unsigned char* new_ptr;
    unsigned new_scanline;

    col_count = 0;

    if (rns_ptr != 0 && rns_size == 6) {
        /* assume 8 bits per pixel */
        col_count = 1;
        col_ptr[0] = rns_ptr[1];
        col_ptr[1] = rns_ptr[3];
        col_ptr[2] = rns_ptr[5];

        *palrns_count = 1;
        palrns_ptr[0] = 0x0;
    } else {
        *palrns_count = 0;
    }

    new_scanline = width;
    new_ptr = data_alloc(height * new_scanline);

    for(i=0;i<height;++i) {
        unsigned char* p0 = img_ptr + i * img_scanline;
        unsigned char* p1 = new_ptr + i * new_scanline;
        for(j=0;j<width;++j) {
            for(k=0;k<col_count;++k) {
                if (col_ptr[k*3] == p0[0] && col_ptr[k*3+1] == p0[1] && col_ptr[k*3+2] == p0[2])
                    break;
            }
            if (k == col_count) {
                if (col_count == 256) {
                    data_free(new_ptr);
                    return 0; /* too many colors */
                }
                col_ptr[col_count*3] = p0[0];
                col_ptr[col_count*3+1] = p0[1];
                col_ptr[col_count*3+2] = p0[2];
                ++col_count;
            }
            *p1 = k;
            ++p1;
            p0 += 3;
        }
    }

    memcpy(pal_ptr, col_ptr, col_count * 3);
    *pal_count = col_count;
    *out_ptr = new_ptr;
    *out_scanline = new_scanline;

    return 1;
}

// 通过png_write对数据进行压缩，并将压缩后的数据放在 第一个参数f中
int write_image(adv_fz* f_out, unsigned pix_width, unsigned pix_height, unsigned pix_pixel, unsigned char* pix_ptr, unsigned pix_scanline, unsigned char* pal_ptr, unsigned pal_size, unsigned char* rns_ptr, unsigned rns_size)
{
    if (pix_pixel == 1) {
        return png_write(f_out, pix_width, pix_height, pix_pixel, pix_ptr, pix_scanline, pal_ptr, pal_size, rns_ptr, rns_size, opt_level);
    } else {
        unsigned char new_pal_ptr[256*3];
        unsigned new_pal_count;
        unsigned char new_rns_ptr[256];
        unsigned new_rns_count;
        unsigned char* new_ptr = NULL;
        unsigned new_scanline;

        int err;

        if (pix_pixel == 3 && reduce_image(&new_ptr, &new_scanline, new_pal_ptr, &new_pal_count, new_rns_ptr, &new_rns_count, pix_width, pix_height, pix_ptr, pix_scanline, rns_ptr, rns_size)) {
            err = png_write(f_out, pix_width, pix_height, 1, new_ptr, new_scanline, new_pal_ptr, new_pal_count * 3, new_rns_count ? new_rns_ptr : 0, new_rns_count, opt_level);
        } else {
            err = png_write(f_out, pix_width, pix_height, pix_pixel, pix_ptr, pix_scanline, 0, 0, rns_ptr, rns_size, opt_level);
        }

        if (new_ptr) data_free(new_ptr);

        return err;
    }
}

/**
 * 接收输入文件信息和临时的输出文件信息 进行压缩
 * f_out: 临时输出文件
 */
bool convert_f(adv_fz* f_in, adv_fz* f_out)
{
    // 图片数据指针
    unsigned char* dat_ptr;
    // 图片大小
    unsigned dat_size;
    // 每个像素占用的字节数 png 32位是8 24位是3 8位1
    unsigned pix_pixel;
    unsigned pix_width;
    unsigned pix_height;
    // 调色板指针
    unsigned char* pal_ptr;
    // 调色板大小
    unsigned pal_size;
    // 透明度指针
    unsigned char* rns_ptr;
    // 透明度大小
    unsigned rns_size;
    // 指向数据的开头指针，其实也就是图片数据指针吧
    unsigned char* pix_ptr;
    // 扫描线的长度
    unsigned pix_scanline;

    if (adv_png_read_rns(
            &pix_width, &pix_height, &pix_pixel,
            &dat_ptr, &dat_size,
            &pix_ptr, &pix_scanline,
            &pal_ptr, &pal_size,
            &rns_ptr, &rns_size,
            f_in) != 0) {
        return false;
    }

    if (write_image(
            f_out,
            pix_width, pix_height, pix_pixel,
            pix_ptr, pix_scanline,
            pal_ptr, pal_size,
            rns_ptr, rns_size) != 0) {
        free(dat_ptr);
        free(pal_ptr);
        free(rns_ptr);
        return false;
    }

    free(dat_ptr);
    free(pal_ptr);
    free(rns_ptr);
    return true;
}

// 向外暴露 二次压缩文件
int rezip_png(uint8_t *input_buffer, size_t input_size, uint8_t **out_image, size_t *out_size,
              int iter, int level) {
    switch (level) {
        case 0:
        case 1:
            // libdeflate
            opt_level.level = shrink_normal;
            break;
        case 2:
        case 3:
            // zopfli
            opt_level.level = shrink_insane;
            break;
        default:
            opt_level.level = shrink_normal;
    }
    iter = iter > 0 ? iter : 1;
    opt_level.iter = iter;

    adv_fz* f_in;
    adv_fz* f_out;
    if (!input_buffer || input_size <= 0) {
        return OPTION_ERROR;
    }
    // 初始化输入输出虚拟文件
    f_in = fzopenmemory(input_buffer, input_size);
    f_out = fzalloc();
    f_out->type = fz_memory_write;
    f_out->virtual_pos = 0;
    f_out->data_write = NULL;
    f_out->virtual_size = 0;

    if (!convert_f(f_in, f_out)) {
        fzclose(f_in);
        free(f_out);
        return ZIP_ERROR;
    }

    *out_image = f_out->data_write;
    *out_size = f_out->virtual_size;

    fzclose(f_in);
    free(f_out);
    return 0;
}

const char *get_version() {
    return version_str;
}
