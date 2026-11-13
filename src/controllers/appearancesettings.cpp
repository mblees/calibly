#include "appearancesettings.h"

#include <QSettings>

#include <algorithm>
#include <cmath>

namespace {

const QString kAccentKey = QStringLiteral("appearance/accentColor");

// Relative luminance (WCAG) of an sRGB color.
double luminance(const QColor &color)
{
    const auto channel = [](double c) {
        return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) + 0.0722 * channel(color.blueF());
}

double contrast(const QColor &a, const QColor &b)
{
    const double la = luminance(a);
    const double lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

} // namespace

AppearanceSettings *AppearanceSettings::s_instance = nullptr;

AppearanceSettings::AppearanceSettings(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_accent(defaultAccentColor())
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    const QColor stored(QSettings().value(kAccentKey).toString());
    if (stored.isValid())
        m_accent = usableAccent(stored);
}

AppearanceSettings::~AppearanceSettings()
{
    s_instance = nullptr;
}

AppearanceSettings *AppearanceSettings::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void AppearanceSettings::setAccentColor(const QColor &color)
{
    const QColor accent = usableAccent(color);
    if (accent == m_accent)
        return;
    m_accent = accent;
    QSettings settings;
    if (isDefault())
        settings.remove(kAccentKey);
    else
        settings.setValue(kAccentKey, m_accent.name(QColor::HexRgb));
    emit accentColorChanged();
}

QColor AppearanceSettings::defaultAccentColor()
{
    return QColor(0xff, 0x8a, 0x3d);
}

double AppearanceSettings::minimumBrightness()
{
    return 0.45;
}

void AppearanceSettings::reset()
{
    setAccentColor(defaultAccentColor());
}

QColor AppearanceSettings::usableAccent(const QColor &color)
{
    if (!color.isValid())
        return defaultAccentColor();
    QColor hsv = color.toHsv();
    if (hsv.valueF() < minimumBrightness())
        hsv.setHsvF(hsv.hsvHueF(), hsv.hsvSaturationF(), minimumBrightness());
    // Through the hex form, so a stored color reads back as the same one.
    return QColor(hsv.toRgb().name(QColor::HexRgb));
}

QColor AppearanceSettings::foregroundFor(const QColor &background)
{
    const QColor rgb = background.toRgb();
    const float hue = std::max(0.0f, rgb.hslHueF());
    const QColor dark = QColor::fromHslF(hue, std::max(0.0f, rgb.hslSaturationF()), 0.06f).toRgb();
    const QColor light(Qt::white);
    return contrast(rgb, dark) >= contrast(rgb, light) ? QColor(dark.name(QColor::HexRgb)) : light;
}
