// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "readersettings.h"

#include <QFontDatabase>
#include <QSettings>

#include <algorithm>
#include <array>

namespace {

constexpr std::array<int, 7> kFontSizes = { 14, 16, 18, 20, 22, 26, 30 };

struct Theme
{
    const char *id;
    const char *family; // "" = the book's own fonts
    bool bold;
    double lineHeight;
    double letterSpacing;
    double wordSpacing;
    int margin;
    const char *pageColor;
};

// Ready combinations, as in the mockup mockups/calibly-reader-hybrid.html.
// "Original" uses the dark page, like the rest of calibly.
constexpr std::array<Theme, 6> kThemes = { {
    { "original", "", false, 1.6, 0.0, 0.0, 56, "nacht" },
    { "ruhig", "Palatino Linotype", false, 1.7, 0.0, 0.0, 64, "grau" },
    { "papier", "Cambria", false, 1.6, 0.0, 0.0, 56, "sepia" },
    { "kraeftig", "Segoe UI", true, 1.5, 0.02, 0.1, 48, "weiss" },
    { "sanft", "Constantia", false, 1.8, 0.02, 0.1, 72, "sepia" },
    { "fokus", "Georgia", false, 1.7, 0.0, 0.0, 104, "nacht" },
} };

const QStringList kPageColors = { QStringLiteral("weiss"), QStringLiteral("sepia"), QStringLiteral("grau"),
                                  QStringLiteral("nacht") };

const QString kGroup = QStringLiteral("reader/");

} // namespace

ReaderSettings::ReaderSettings(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    m_applying = true; // the defaults and reading back are no changes by hand
    reset();
    QSettings settings;
    if (!settings.contains(kGroup + QStringLiteral("fontSizeStep"))) {
        m_applying = false;
        return;
    }
    setFontSizeStep(settings.value(kGroup + QStringLiteral("fontSizeStep"), m_fontSizeStep).toInt());
    setPageColor(settings.value(kGroup + QStringLiteral("pageColor"), m_pageColor).toString());
    setFontFamily(settings.value(kGroup + QStringLiteral("fontFamily"), m_fontFamily).toString());
    setBold(settings.value(kGroup + QStringLiteral("bold"), m_bold).toBool());
    setLineHeight(settings.value(kGroup + QStringLiteral("lineHeight"), m_lineHeight).toDouble());
    setLetterSpacing(settings.value(kGroup + QStringLiteral("letterSpacing"), m_letterSpacing).toDouble());
    setWordSpacing(settings.value(kGroup + QStringLiteral("wordSpacing"), m_wordSpacing).toDouble());
    setMargin(settings.value(kGroup + QStringLiteral("margin"), m_margin).toInt());
    setJustify(settings.value(kGroup + QStringLiteral("justify"), m_justify).toBool());
    setTwoPages(settings.value(kGroup + QStringLiteral("twoPages"), m_twoPages).toBool());
    m_theme = settings.value(kGroup + QStringLiteral("theme"), m_theme).toString();
    m_applying = false;
}

ReaderSettings::~ReaderSettings()
{
    s_instance = nullptr;
}

ReaderSettings *ReaderSettings::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

int ReaderSettings::fontSizeSteps() const
{
    return int(kFontSizes.size());
}

int ReaderSettings::fontPixelSize() const
{
    return kFontSizes.at(std::size_t(m_fontSizeStep));
}

QStringList ReaderSettings::themes() const
{
    QStringList ids;
    for (const Theme &theme : kThemes)
        ids.append(QString::fromLatin1(theme.id));
    return ids;
}

QStringList ReaderSettings::fontFamilies() const
{
    static const QStringList wanted = { QStringLiteral("Georgia"), QStringLiteral("Palatino Linotype"),
                                        QStringLiteral("Cambria"), QStringLiteral("Constantia"),
                                        QStringLiteral("Segoe UI"), QStringLiteral("Verdana") };
    QStringList available;
    for (const QString &family : wanted) {
        if (QFontDatabase::hasFamily(family))
            available.append(family);
    }
    return available;
}

void ReaderSettings::setFontSizeStep(int step)
{
    step = std::clamp(step, 0, fontSizeSteps() - 1);
    if (step == m_fontSizeStep)
        return;
    m_fontSizeStep = step;
    // The font size is not part of a theme, so the theme stays.
    if (!m_applying)
        store();
    emit changed();
}

#define SETTER(Type, name, member, normalize)                  \
    void ReaderSettings::name(Type value)                       \
    {                                                           \
        value = normalize;                                      \
        if (value == member)                                    \
            return;                                             \
        member = value;                                         \
        changedByHand();                                        \
    }

SETTER(bool, setBold, m_bold, value)
SETTER(bool, setJustify, m_justify, value)
SETTER(int, setMargin, m_margin, std::clamp(value, 24, 120))

#undef SETTER

void ReaderSettings::setPageColor(const QString &color)
{
    if (!kPageColors.contains(color) || color == m_pageColor)
        return;
    m_pageColor = color;
    changedByHand();
}

void ReaderSettings::setFontFamily(const QString &family)
{
    if (family == m_fontFamily)
        return;
    m_fontFamily = family;
    changedByHand();
}

void ReaderSettings::setLineHeight(double lineHeight)
{
    lineHeight = std::clamp(lineHeight, 1.2, 2.2);
    if (qFuzzyCompare(lineHeight, m_lineHeight))
        return;
    m_lineHeight = lineHeight;
    changedByHand();
}

void ReaderSettings::setLetterSpacing(double spacing)
{
    spacing = std::clamp(spacing, 0.0, 0.12);
    if (qFuzzyCompare(spacing + 1, m_letterSpacing + 1))
        return;
    m_letterSpacing = spacing;
    changedByHand();
}

void ReaderSettings::setWordSpacing(double spacing)
{
    spacing = std::clamp(spacing, 0.0, 0.5);
    if (qFuzzyCompare(spacing + 1, m_wordSpacing + 1))
        return;
    m_wordSpacing = spacing;
    changedByHand();
}

void ReaderSettings::setTwoPages(bool twoPages)
{
    if (twoPages == m_twoPages)
        return;
    m_twoPages = twoPages;
    // Not a matter of the text: the theme stays.
    if (!m_applying)
        store();
    emit changed();
}

void ReaderSettings::changedByHand()
{
    if (!m_applying) {
        m_theme.clear();
        store();
    }
    emit changed();
}

void ReaderSettings::applyTheme(const QString &id)
{
    const auto it = std::find_if(kThemes.cbegin(), kThemes.cend(),
                                 [&id](const Theme &theme) { return id == QLatin1String(theme.id); });
    if (it == kThemes.cend())
        return;
    const bool wasApplying = m_applying;
    m_applying = true;
    const QString family = QString::fromLatin1(it->family);
    setFontFamily(family.isEmpty() || QFontDatabase::hasFamily(family) ? family : QString());
    setBold(it->bold);
    setLineHeight(it->lineHeight);
    setLetterSpacing(it->letterSpacing);
    setWordSpacing(it->wordSpacing);
    setMargin(it->margin);
    setJustify(true);
    setPageColor(QString::fromLatin1(it->pageColor));
    m_theme = id;
    m_applying = wasApplying;
    if (!m_applying)
        store();
    emit changed();
}

QVariantMap ReaderSettings::themeInfo(const QString &id) const
{
    const auto it = std::find_if(kThemes.cbegin(), kThemes.cend(),
                                 [&id](const Theme &theme) { return id == QLatin1String(theme.id); });
    if (it == kThemes.cend())
        return {};
    return { { QStringLiteral("fontFamily"), QString::fromLatin1(it->family) },
             { QStringLiteral("bold"), it->bold },
             { QStringLiteral("pageColor"), QString::fromLatin1(it->pageColor) } };
}

void ReaderSettings::reset()
{
    const bool wasApplying = m_applying;
    m_applying = true;
    setFontSizeStep(2);
    setTwoPages(false);
    applyTheme(QStringLiteral("original"));
    m_applying = wasApplying;
    if (!m_applying)
        store();
    emit changed();
}

void ReaderSettings::store()
{
    QSettings settings;
    settings.setValue(kGroup + QStringLiteral("fontSizeStep"), m_fontSizeStep);
    settings.setValue(kGroup + QStringLiteral("theme"), m_theme);
    settings.setValue(kGroup + QStringLiteral("pageColor"), m_pageColor);
    settings.setValue(kGroup + QStringLiteral("fontFamily"), m_fontFamily);
    settings.setValue(kGroup + QStringLiteral("bold"), m_bold);
    settings.setValue(kGroup + QStringLiteral("lineHeight"), m_lineHeight);
    settings.setValue(kGroup + QStringLiteral("letterSpacing"), m_letterSpacing);
    settings.setValue(kGroup + QStringLiteral("wordSpacing"), m_wordSpacing);
    settings.setValue(kGroup + QStringLiteral("margin"), m_margin);
    settings.setValue(kGroup + QStringLiteral("justify"), m_justify);
    settings.setValue(kGroup + QStringLiteral("twoPages"), m_twoPages);
}

TextStyle ReaderSettings::textStyle() const
{
    TextStyle style;
    style.fontPixelSize = fontPixelSize();
    style.fontFamily = m_fontFamily;
    style.bold = m_bold;
    style.lineHeight = m_lineHeight;
    style.letterSpacing = m_letterSpacing;
    style.wordSpacing = m_wordSpacing;
    style.margin = m_margin;
    style.justify = m_justify;
    return style;
}
