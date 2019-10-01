#ifndef CONFIG_H
#define CONFIG_H

#include <QMetaType>
#include <QString>

#include <jpeg-recompress.h>


class Config
{
public:
    Config();

    operator jpeg_recompress_config_t() const;
    operator const jpeg_recompress_config_t*() const;

    jpeg_recompress_quality_t quality() const;
    void setQuality(jpeg_recompress_quality_t quality);

    image_compare_method_t method() const;
    void setMethod(image_compare_method_t method);

    bool strip() const;
    void setStrip(bool strip);

    int jpegMin() const;
    void setJpegMin(int jpegMin);

    int jpegMax() const;
    void setJpegMax(int jpegMax);

    int attempts() const;
    void setAttempts(int attempts);

    bool progressive() const;
    void setProgressive(bool progressive);

    bool copy() const;
    void setCopy(bool copy);

    bool overwrite() const;
    void setOverwrite(bool overwrite);

private:
    bool m_overwrite = false;
    jpeg_recompress_quality_t m_quality;
    jpeg_recompress_config_t m_config;
};

Q_DECLARE_METATYPE(jpeg_recompress_quality_t);
Q_DECLARE_METATYPE(image_compare_method_t);

#endif // CONFIG_H
