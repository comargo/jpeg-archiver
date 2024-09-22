#include "image_compare.h"

#include <stdlib.h>

extern "C" {
#include <iqa.h>
}

static double smallfry_metric(const void *image1, const void *image2,
                              unsigned int width, unsigned int height)
{
    return 0;
}

static double mean_pixel_error_metric(const void *image1, const void *image2,
                                      unsigned int width, unsigned int height, int k)
{
    return 0;
}

double image_compare(const unsigned char *image1, const unsigned char *image2,
                     unsigned int width, unsigned int height,
                     enum image_compare_method_t method)
{
    switch (method) {
    case IC_METHOD_SSIM:
        return (double)iqa_ssim(image1, image2, (int)width, (int)height, (int)width, 0, NULL);
    case IC_METHOD_MS_SSIM:
        return (double)iqa_ms_ssim(image1, image2, (int)width, (int)height, (int)width, NULL);
    case IC_METHOD_SMALLFRY:
        return smallfry_metric(image1, image2, width, height);
    case IC_METHOD_MPE:
        return mean_pixel_error_metric(image1, image2, width, height, 1);
    default:
        return 0;
    }
}
