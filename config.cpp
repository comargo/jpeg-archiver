#include "config.h"

Config::Config()
{

}

Config::QualityPreset Config::quality() const
{
    return m_quality;
}

void Config::setQuality(const Config::QualityPreset &quality)
{
    m_quality = quality;
}

Config::Method Config::method() const
{
    return m_method;
}

void Config::setMethod(const Config::Method &method)
{
    m_method = method;
}

bool Config::strip() const
{
    return m_strip;
}

void Config::setStrip(bool strip)
{
    m_strip = strip;
}
