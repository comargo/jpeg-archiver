#include "config.h"

Config::Config()
{
    m_quality = JR_QUALITY_MEDIUM;
    jr_default_config(&m_config);
}

Config::operator jpeg_recompress_config_t() const
{
    return m_config;
}

Config::operator const jpeg_recompress_config_t *() const
{
    return &m_config;
}

jpeg_recompress_quality_t Config::quality() const
{
    return m_quality;
}

void Config::setQuality(jpeg_recompress_quality_t quality)
{
    m_quality = quality;
    m_config.target = jr_get_default_target(m_quality, m_config.method);
}

image_compare_method_t Config::method() const
{
    return m_config.method;
}

void Config::setMethod(image_compare_method_t method)
{
    m_config.method = method;
    m_config.target = jr_get_default_target(m_quality, m_config.method);
}

bool Config::strip() const
{
    return m_config.strip;
}

void Config::setStrip(bool strip)
{
    m_config.strip = strip;
}

int Config::jpegMin() const
{
    return static_cast<int>(m_config.quality_min);
}

void Config::setJpegMin(int jpegMin)
{
    m_config.quality_min = static_cast<uint>(jpegMin);
}

int Config::jpegMax() const
{
    return static_cast<int>(m_config.quality_max);
}

void Config::setJpegMax(int jpegMax)
{
    m_config.quality_max = static_cast<uint>(jpegMax);
}

int Config::attempts() const
{
    return static_cast<int>(m_config.attempts);
}

void Config::setAttempts(int attempts)
{
    m_config.attempts = static_cast<uint>(attempts);
}

bool Config::progressive() const
{
    return !m_config.no_progressive;
}

void Config::setProgressive(bool progressive)
{
    m_config.no_progressive = !progressive;
}

bool Config::copy() const
{
    return m_config.copy;
}

void Config::setCopy(bool copy)
{
    m_config.copy = copy;
}

bool Config::overwrite() const
{
    return m_overwrite;
}

void Config::setOverwrite(bool overwrite)
{
    m_overwrite = overwrite;
}
