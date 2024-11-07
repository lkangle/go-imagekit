#define _XOPEN_SOURCE 700
#include "quant.h"
#include "lib/libimagequant.h"
#include "libadvpng/advpng.h"
#include "rwpng.h" /* typedefs, common macros, public prototypes */
#include <stdbool.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_max_threads() 1
#define omp_get_thread_num() 0
#endif

// 图片解码后的基础信息
struct decode_info
{
    size_t size;
    int width;
    int height;
};

struct pngquant_options
{
    liq_attr *liq;
    liq_image *fixed_palette_image;
    float floyd;
    bool fast_compression;
    bool skip_if_larger;
};

static void set_palette(liq_result *result, png8_image *output_image)
{
    const liq_palette *palette = liq_get_palette(result);

    // tRNS, etc.
    output_image->num_palette = palette->count;
    output_image->num_trans = 0;
    for (unsigned int i = 0; i < palette->count; i++)
    {
        liq_color px = palette->entries[i];
        if (px.a < 255)
        {
            output_image->num_trans = i + 1;
        }
        output_image->palette[i] = (png_color){.red = px.r, .green = px.g, .blue = px.b};
        output_image->trans[i] = px.a;
    }
}

static pngquant_error write_image(png8_image *output_image, unsigned char **out, size_t *size)
{
    FILE *outfile;

    if ((outfile = open_memstream((char **)out, size)) == NULL)
    {
        return CANT_WRITE_ERROR;
    }

    pngquant_error retval = SUCCESS;
#pragma omp critical(libpng)
    {
        if (output_image)
        {
            retval = rwpng_write_image8(outfile, output_image);
        }
    }

    fclose(outfile);
    return retval;
}

static pngquant_error read_image(liq_attr *options, void *file_buffer, size_t file_size,
                                 png24_image *input_image_p, liq_image **liq_image_p,
                                 bool keep_input_pixels)
{
    FILE *infile;

    if ((infile = fmemopen(file_buffer, file_size, "rb")) == NULL)
    {
        return READ_ERROR;
    }

    pngquant_error retval;
#pragma omp critical(libpng)
    {
        retval = rwpng_read_image24(infile, input_image_p, 0);
    }

    fclose(infile);

    if (retval)
    {
        return retval;
    }

    *liq_image_p = liq_image_create_rgba_rows(options, (void **)input_image_p->row_pointers, input_image_p->width, input_image_p->height, input_image_p->gamma);

    if (!*liq_image_p)
    {
        return OUT_OF_MEMORY_ERROR;
    }

    if (!keep_input_pixels)
    {
        if (LIQ_OK != liq_image_set_memory_ownership(*liq_image_p, LIQ_OWN_ROWS | LIQ_OWN_PIXELS))
        {
            return OUT_OF_MEMORY_ERROR;
        }
        input_image_p->row_pointers = NULL;
        input_image_p->rgba_data = NULL;
    }

    return SUCCESS;
}

static pngquant_error prepare_output_image(liq_result *result, liq_image *input_image, png8_image *output_image)
{
    output_image->width = liq_image_get_width(input_image);
    output_image->height = liq_image_get_height(input_image);
    output_image->gamma = liq_get_output_gamma(result);

    /*
    ** Step 3.7 [GRR]: allocate memory for the entire indexed image
    */

    output_image->indexed_data = malloc(output_image->height * output_image->width);
    output_image->row_pointers = malloc(output_image->height * sizeof(output_image->row_pointers[0]));

    if (!output_image->indexed_data || !output_image->row_pointers)
    {
        return OUT_OF_MEMORY_ERROR;
    }

    for (unsigned int row = 0; row < output_image->height; ++row)
    {
        output_image->row_pointers[row] = output_image->indexed_data + row * output_image->width;
    }

    const liq_palette *palette = liq_get_palette(result);
    // tRNS, etc.
    output_image->num_palette = palette->count;
    output_image->num_trans = 0;
    for (unsigned int i = 0; i < palette->count; i++)
    {
        if (palette->entries[i].a < 255)
        {
            output_image->num_trans = i + 1;
        }
    }

    return SUCCESS;
}

pngquant_error pngquant_file(unsigned char *input_buffer, size_t input_size,
                             unsigned char **out_buffer, struct decode_info *dinfo, struct pngquant_options *options)
{
    pngquant_error retval = SUCCESS;
    size_t outsize;

    liq_image *input_image = NULL;
    png24_image input_image_rwpng = {};
    bool keep_input_pixels = options->skip_if_larger; // original may need to be output to stdout
    retval = read_image(options->liq, input_buffer, input_size, &input_image_rwpng, &input_image, keep_input_pixels);

    // quality on 0-100 scale, updated upon successful remap
    int quality_percent = 90;
    png8_image output_image = {};
    if (SUCCESS == retval)
    {
        // when using image as source of a fixed palette the palette is extracted using regular quantization
        liq_result *remap = liq_quantize_image(options->liq, options->fixed_palette_image ? options->fixed_palette_image : input_image);

        if (remap)
        {
            // fixed gamma ~2.2 for the web. PNG can't store exact 1/2.2
            liq_set_output_gamma(remap, 0.45455);
            liq_set_dithering_level(remap, options->floyd);

            retval = prepare_output_image(remap, input_image, &output_image);
            if (SUCCESS == retval)
            {
                if (LIQ_OK != liq_write_remapped_image_rows(remap, input_image, output_image.row_pointers))
                {
                    retval = OUT_OF_MEMORY_ERROR;
                }

                set_palette(remap, &output_image);

                double palette_error = liq_get_quantization_error(remap);
                if (palette_error >= 0)
                {
                    quality_percent = liq_get_quantization_quality(remap);
                }
            }
            liq_result_destroy(remap);
        }
        else
        {
            retval = TOO_LOW_QUALITY;
        }
    }

    if (SUCCESS == retval)
    {
        if (options->skip_if_larger)
        {
            // this is very rough approximation, but generally avoid losing more quality than is gained in file size.
            // Quality is squared, because even greater savings are needed to justify big quality loss.
            double quality = quality_percent / 100.0;
            output_image.maximum_file_size = (input_image_rwpng.file_size - 1) * quality * quality;
        }

        output_image.fast_compression = options->fast_compression;
        output_image.chunks = input_image_rwpng.chunks;
        input_image_rwpng.chunks = NULL;
        retval = write_image(&output_image, out_buffer, &outsize);
    }

    if (!retval)
    {
        dinfo->size = outsize;
        dinfo->width = (int)input_image_rwpng.width;
        dinfo->height = (int)input_image_rwpng.height;
    }

    liq_image_destroy(input_image);
    rwpng_free_image24(&input_image_rwpng);
    rwpng_free_image8(&output_image);
    return retval;
}

int PNGCompress(unsigned char *input, size_t input_size, unsigned char **out, size_t *outsize, int *width, int *height,
                int min_quality, int max_quality, float floyd, int speed, int rzlevel)
{
    struct pngquant_options options = {
        .fast_compression = false,
        .skip_if_larger = false,
        .liq = liq_attr_create()};
    if (!options.liq)
        return WRONG_ARCHITECTURE;

    if (floyd < 0 || floyd > 1.f)
    {
        options.floyd = 1.f;
    }
    else
    {
        options.floyd = floyd;
    }

    if (speed >= 10)
    {
        options.fast_compression = true;
    }
    if (speed == 11)
    {
        options.floyd = 0;
        speed = 10;
    }
    if (speed <= 0)
    {
        speed = 3;
    }
    if (LIQ_OK != liq_set_speed(options.liq, speed))
        return NOT_OVERWRITING_ERROR;

    options.fixed_palette_image = NULL;
    if (LIQ_OK != liq_set_quality(options.liq, min_quality, max_quality))
        return NOT_OVERWRITING_ERROR;

#ifdef _OPENMP
    omp_set_nested(1);
#endif

    unsigned char *out_buffer;

    struct decode_info dinfo;
    int retval = pngquant_file(input, input_size, &out_buffer, &dinfo, &options);
    if (!retval)
    {
        *out = out_buffer;
        *outsize = dinfo.size;
        *width = dinfo.width;
        *height = dinfo.height;
    }

    liq_image_destroy(options.fixed_palette_image);
    liq_attr_destroy(options.liq);

    // 二次压缩
    if (rzlevel != 0 && !retval)
    {
        retval = rezip_png(*out, *outsize, out, outsize, 1, rzlevel);
    }

    return retval;
}

int PNGDecode(unsigned char *input, size_t input_size,
              unsigned char **rgba, int *width, int *height)
{
    png24_image input_image_rwpng = {};

    FILE *infile;

    if ((infile = fmemopen(input, input_size, "rb")) == NULL)
    {
        return READ_ERROR;
    }

    pngquant_error retval;
    {
        retval = rwpng_read_image24(infile, &input_image_rwpng, 0);
    }

    fclose(infile);

    if (retval)
    {
        return retval;
    }

    *rgba = input_image_rwpng.rgba_data;
    *width = (int)input_image_rwpng.width;
    *height = (int)input_image_rwpng.height;

    input_image_rwpng.rgba_data = NULL;

    rwpng_free_image24(&input_image_rwpng);
    return retval;
}
