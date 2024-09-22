#ifndef IMAGE_COMPARE_H
#define IMAGE_COMPARE_H

#ifdef __cplusplus
extern "C" {
#endif

enum image_compare_method_t {
    IC_METHOD_SSIM = 0,
    IC_METHOD_MS_SSIM,
    IC_METHOD_SMALLFRY,
    IC_METHOD_MPE
};

double image_compare(const unsigned char *image1, const unsigned char *image2,
                     unsigned int width, unsigned int height,
                     enum image_compare_method_t method);

#ifdef __cplusplus
}
#endif
#endif//IMAGE_COMPARE_H
