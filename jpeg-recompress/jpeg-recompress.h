#ifndef JPEG_RECOMPRESS_H
#define JPEG_RECOMPRESS_H

#include <stdbool.h>
#include "image_compare.h"

#ifdef __cplusplus
extern "C" {
#endif

enum jpeg_recompress_quality_t {
    JR_QUALITY_LOW = 0,
    JR_QUALITY_MEDIUM,
    JR_QUALITY_HIGH,
    JR_QUALITY_VERYHIGH
};

struct jpeg_recompress_config_t {
    double target;
    unsigned int quality_min;
    unsigned int quality_max;
    unsigned int attempts;
    bool accurate;
    enum image_compare_method_t method;
    bool no_progressive;
    bool strip;
    bool copy;
};

void jr_default_config(struct jpeg_recompress_config_t *config);

double jr_get_default_target(enum jpeg_recompress_quality_t quality, enum image_compare_method_t method);

int jpeg_recompress(const void *in_jpeg, size_t in_jpeg_size, const struct jpeg_recompress_config_t *config,
                             unsigned char **out_jpeg, size_t *out_jpeg_size);


#ifdef __cplusplus
}
#endif
#endif//JPEG_RECOMPRESS_H
