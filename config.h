#ifndef CONFIG_H
#define CONFIG_H

#include <QMetaType>
#include <QString>


class Config
{
public:
    enum class QualityPreset {
        Low,
        Medium,
        High,
        VeryHigh
    };
    enum class Method {
        SSIM,
        MS_SSIM,
        SMALLFRY,
        MPE
    };

public:
    Config();

    QualityPreset quality() const;
    void setQuality(const QualityPreset &quality);

    Method method() const;
    void setMethod(const Method &method);

    bool strip() const;
    void setStrip(bool strip);

private:
    QualityPreset m_quality = QualityPreset::Medium;
    Method m_method = Method::SSIM;
    bool m_strip = false;
};

Q_DECLARE_METATYPE(Config::QualityPreset);
Q_DECLARE_METATYPE(Config::Method);

#endif // CONFIG_H
