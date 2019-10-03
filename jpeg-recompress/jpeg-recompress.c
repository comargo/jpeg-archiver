#include "jpeg-recompress.h"

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <jpeglib.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static const char *COMMENT = "Compressed by jpeg-recompress";

double jr_get_default_target(enum jpeg_recompress_quality_t quality, enum image_compare_method_t method)
{
    static const double default_targets[4][4] = {
    /*   LOW     MEDIUM  HIGH     VERYHIGH */
        {0.999,  0.9999, 0.99995, 0.99999}, /* SSIM */
        {0.85,   0.94,   0.96,    0.98}, /* MS-SSIM */
        {100.75, 102.25, 103.8,   105.5}, /* SMALLFRY */
        {1.5,    1.0,    0.8,     0.6}
    };

    return default_targets[method][quality];
}


static size_t decompress_jpeg(struct jpeg_decompress_struct *dinfo, const void *in_jpeg, size_t in_jpeg_size,
                            J_COLOR_SPACE out_color_space, unsigned char *output_image)
{
    JSAMPARRAY buffer;

    jpeg_mem_src(dinfo, in_jpeg, (unsigned long)in_jpeg_size);
    jpeg_read_header(dinfo, TRUE);
    dinfo->out_color_space = out_color_space;
    jpeg_start_decompress(dinfo);
    size_t row_stride = dinfo->output_width*((unsigned int)dinfo->output_components);
    if(output_image) {
        buffer = (*dinfo->mem->alloc_sarray)
                ((j_common_ptr) dinfo, JPOOL_IMAGE, (JDIMENSION)row_stride, 1);
        if(!output_image)
            output_image = malloc(row_stride*dinfo->output_height);
        while (dinfo->output_scanline < dinfo->output_height) {
            jpeg_read_scanlines(dinfo, buffer,1);
            memcpy(output_image+(dinfo->output_scanline-1)*row_stride, buffer[0], row_stride);
        }
        jpeg_finish_decompress(dinfo);
    }
    else {
        jpeg_abort_decompress(dinfo);
    }
    return row_stride;
}

jpeg_saved_marker_ptr copy_marker_list(jpeg_saved_marker_ptr marker_list)
{
    if(marker_list == NULL)
        return NULL;
    jpeg_saved_marker_ptr new_list = malloc(sizeof(struct jpeg_marker_struct));
    new_list->marker = marker_list->marker;
    new_list->original_length = marker_list->original_length;
    new_list->data_length = marker_list->data_length;
    new_list->data = malloc(marker_list->data_length);
    memcpy(new_list->data, marker_list->data, marker_list->data_length);
    new_list->next = copy_marker_list(marker_list->next);
    return new_list;
}

void destroy_marker_list(jpeg_saved_marker_ptr marker_list)
{
    if(marker_list == NULL)
        return;
    destroy_marker_list(marker_list->next);
    free(marker_list->data);
    free(marker_list);
}

int jpeg_recompress(const void *in_jpeg, size_t in_jpeg_size, const struct jpeg_recompress_config_t *config,
                             unsigned char **out_jpeg, size_t *out_jpeg_size)
{
    unsigned int quality = 0;

    struct jpeg_decompress_struct dinfo;
    struct jpeg_error_mgr err;
    dinfo.err = jpeg_std_error(&err);
    jpeg_create_decompress(&dinfo);

    jpeg_save_markers(&dinfo, JPEG_COM, UINT_MAX);
    for(int i=0; i<16; ++i) {
        jpeg_save_markers(&dinfo, JPEG_APP0+i, UINT_MAX);
    }
    jpeg_mem_src(&dinfo, in_jpeg, (unsigned long)in_jpeg_size);
    jpeg_read_header(&dinfo, TRUE);
    struct jpeg_marker_struct marker_list = {0};
    marker_list.next = copy_marker_list(dinfo.marker_list);
    jpeg_abort_decompress(&dinfo);

    bool already_processed = FALSE;
    for(jpeg_saved_marker_ptr marker = marker_list.next, prev = &marker_list; marker != NULL; marker = marker->next)
    {
        if(marker->marker == JPEG_COM) {
            if(strncmp((const char*)marker->data, COMMENT, strlen(COMMENT)) == 0)
                already_processed = TRUE;
            free(marker->data);
            prev->next = marker->next; // Link prev with next
            free(marker);
            marker = prev; // Step back
        }
        prev = marker;
    }

    if(config->strip) {
        destroy_marker_list(marker_list.next);
        marker_list.next = NULL;
    }
    if(already_processed) {
        destroy_marker_list(marker_list.next);
        jpeg_destroy_decompress(&dinfo);

        if(config->copy) {
            if(out_jpeg) {
                *out_jpeg = malloc(in_jpeg_size);
                memcpy(*out_jpeg, in_jpeg, in_jpeg_size);
            }
            if(out_jpeg_size) {
                *out_jpeg_size = in_jpeg_size;
            }
        }
        return  0;
    }

    size_t rgb_row_stride = decompress_jpeg(&dinfo, in_jpeg, in_jpeg_size, JCS_RGB, NULL);
    unsigned char *original_rgb = malloc(dinfo.output_height*rgb_row_stride);
    decompress_jpeg(&dinfo, in_jpeg, in_jpeg_size, JCS_RGB, original_rgb);

    size_t gray_row_stride = decompress_jpeg(&dinfo, in_jpeg, in_jpeg_size, JCS_GRAYSCALE, NULL);
    unsigned char *original_grayscale = malloc(dinfo.output_height*gray_row_stride);
    decompress_jpeg(&dinfo, in_jpeg, in_jpeg_size, JCS_GRAYSCALE, original_grayscale);

    unsigned char* compressed = NULL;
    size_t compressed_buf_size = 0;
    unsigned long compressed_size = 0;
    unsigned char *decompressed = malloc(gray_row_stride*dinfo.output_height);

    unsigned int jpeg_min = config->quality_min;
    unsigned int jpeg_max = config->quality_max;

    struct jpeg_compress_struct cinfo;
    cinfo.err = jpeg_std_error(&err);
    jpeg_create_compress(&cinfo);


    for(unsigned int attempt = 0; attempt<config->attempts; ++attempt) {
	if(compressed) {
	  free(compressed);
	  compressed=NULL;
	  compressed_size = 0;
	}
        jpeg_mem_dest(&cinfo, &compressed, &compressed_size);
        double metric;
        quality = (jpeg_min + jpeg_max)/2;
        bool last_attempt = (attempt+1 == config->attempts);
        if(jpeg_min == jpeg_max)
            last_attempt = true;
        bool progressive = last_attempt?!config->no_progressive:false;
        bool optimize = (config->accurate || last_attempt);
        cinfo.image_width = dinfo.output_width;
        cinfo.image_height = dinfo.output_height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        if(!optimize) {
            if (jpeg_c_int_param_supported(&cinfo, JINT_COMPRESS_PROFILE)) {
                jpeg_c_set_int_param(&cinfo, JINT_COMPRESS_PROFILE, JCP_FASTEST);
            }
        }
        jpeg_set_defaults(&cinfo);

        if(!optimize) {
            if (jpeg_c_bool_param_supported(&cinfo, JBOOLEAN_TRELLIS_QUANT)) {
                jpeg_c_set_bool_param(&cinfo, JBOOLEAN_TRELLIS_QUANT, FALSE);
            }
            if (jpeg_c_bool_param_supported(&cinfo, JBOOLEAN_TRELLIS_QUANT_DC)) {
                jpeg_c_set_bool_param(&cinfo, JBOOLEAN_TRELLIS_QUANT_DC, FALSE);
            }
        }

        if (optimize && !progressive) {
            // Moz defaults, disable progressive
            cinfo.scan_info = NULL;
            cinfo.num_scans = 0;
            if (jpeg_c_bool_param_supported(&cinfo, JBOOLEAN_OPTIMIZE_SCANS)) {
                jpeg_c_set_bool_param(&cinfo, JBOOLEAN_OPTIMIZE_SCANS, FALSE);
            }
        }

        if (!optimize && progressive) {
            // No moz defaults, set scan progression
            jpeg_simple_progression(&cinfo);
        }

        jpeg_set_quality(&cinfo, (int)quality, TRUE);
        if(last_attempt) {
            cinfo.density_unit = dinfo.density_unit;
            cinfo.X_density = dinfo.X_density;
            cinfo.Y_density = dinfo.Y_density;
            cinfo.write_Adobe_marker = dinfo.saw_Adobe_marker;
        }
        jpeg_start_compress(&cinfo, TRUE);
        if(last_attempt) {
            for(jpeg_saved_marker_ptr marker = marker_list.next; marker != NULL; marker = marker->next)
            {
                jpeg_write_marker(&cinfo, marker->marker, marker->data, marker->data_length);
            }
            jpeg_write_marker(&cinfo, JPEG_COM, (const JOCTET*)COMMENT, (unsigned int)strlen(COMMENT));
        }
        JSAMPROW row_pointer[1];
        while (cinfo.next_scanline < cinfo.image_height) {
            row_pointer[0] = &original_rgb[cinfo.next_scanline * rgb_row_stride];
            (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
        jpeg_finish_compress(&cinfo);
        compressed_buf_size = compressed_size + cinfo.dest->free_in_buffer;


        decompress_jpeg(&dinfo, compressed, compressed_size, JCS_GRAYSCALE, decompressed);

        metric = image_compare(original_grayscale, decompressed,
                               dinfo.output_width, dinfo.output_height, config->method);

        if(metric < config->target) {
            if(compressed_size >= in_jpeg_size) {
                last_attempt = TRUE;
                if(config->copy) {
                    compressed_size = (unsigned long)in_jpeg_size;
                    memcpy(compressed, in_jpeg, in_jpeg_size);
                }
                else {
                    free(compressed);
                    compressed = NULL;
                    compressed_size = 0;
                }
            }
            switch (config->method) {
            case IC_METHOD_SSIM:
            case IC_METHOD_MS_SSIM:
            case IC_METHOD_SMALLFRY:
                jpeg_min = MIN(quality+1, jpeg_max);
                break;
            case IC_METHOD_MPE:
                jpeg_max = MAX(quality-1, jpeg_min);
                break;
            }
        }
        else {
            switch (config->method) {
            case IC_METHOD_SSIM:
            case IC_METHOD_MS_SSIM:
            case IC_METHOD_SMALLFRY:
                jpeg_max = MAX(quality-1, jpeg_min);
                break;
            case IC_METHOD_MPE:
                jpeg_min = MIN(quality+1, jpeg_max);
                break;
            }
        }
        if(last_attempt)
            break;
    }
    if(out_jpeg) {
        *out_jpeg = compressed;
    }
    if(out_jpeg_size) {
        *out_jpeg_size = compressed_size;
    }
    else {
        free(compressed);
    }
    free(decompressed);

    free(original_grayscale);
    free(original_rgb);
    destroy_marker_list(marker_list.next);
    jpeg_destroy_decompress(&dinfo);
    jpeg_destroy_compress(&cinfo);
    return quality;
}

void jr_default_config(struct jpeg_recompress_config_t *config)
{
    config->quality_min = 40;
    config->quality_max = 95;
    config->attempts = 6;
    config->accurate = 0;
    config->method = IC_METHOD_SSIM;
    config->no_progressive = 0;
    config->target = jr_get_default_target(JR_QUALITY_MEDIUM, config->method);
    config->strip = false;
    config->copy = true;
}
