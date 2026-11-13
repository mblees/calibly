// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "booklayout.h"

// The text settings of the reader ("Aa"), modelled on Apple Books. They
// apply to all EPUBs and text books and are kept between starts.
//
// In QML (import Calibly.Controllers):
//   ReaderSettings.fontSizeStep      0 … fontSizeSteps - 1 (A small / A large)
//   ReaderSettings.smaller(), larger()
//   ReaderSettings.theme             id of the chosen theme, "" = changed by hand
//   ReaderSettings.themes            ids: original, ruhig, papier, kraeftig, sanft, fokus
//   ReaderSettings.applyTheme(id)    sets the values below (not the font size)
//   ReaderSettings.themeInfo(id)     { fontFamily ("" = the book's), bold, pageColor }
//                                    of a theme, for its button
//   ReaderSettings.pageColor         "weiss", "sepia", "grau" or "nacht"
//   ReaderSettings.fontFamily        "" = the book's own fonts ("Original")
//   ReaderSettings.fontFamilies      the fonts to choose from (those installed)
//   ReaderSettings.bold, lineHeight (1.2 – 2.2), letterSpacing (0 – 0.12 em),
//   wordSpacing (0 – 0.5 em), margin (24 – 120 px), justify
//   ReaderSettings.twoPages          two pages side by side in fullscreen
//   ReaderSettings.reset()           theme "original", middle font size
// Changing a value by hand sets theme to "".
class ReaderSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int fontSizeStep READ fontSizeStep WRITE setFontSizeStep NOTIFY changed)
    Q_PROPERTY(int fontSizeSteps READ fontSizeSteps CONSTANT)
    Q_PROPERTY(QString theme READ theme NOTIFY changed)
    Q_PROPERTY(QStringList themes READ themes CONSTANT)
    Q_PROPERTY(QString pageColor READ pageColor WRITE setPageColor NOTIFY changed)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY changed)
    Q_PROPERTY(QStringList fontFamilies READ fontFamilies CONSTANT)
    Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY changed)
    Q_PROPERTY(double lineHeight READ lineHeight WRITE setLineHeight NOTIFY changed)
    Q_PROPERTY(double letterSpacing READ letterSpacing WRITE setLetterSpacing NOTIFY changed)
    Q_PROPERTY(double wordSpacing READ wordSpacing WRITE setWordSpacing NOTIFY changed)
    Q_PROPERTY(int margin READ margin WRITE setMargin NOTIFY changed)
    Q_PROPERTY(bool justify READ justify WRITE setJustify NOTIFY changed)
    Q_PROPERTY(bool twoPages READ twoPages WRITE setTwoPages NOTIFY changed)

public:
    explicit ReaderSettings(QQmlEngine *engine, QObject *parent = nullptr);
    ~ReaderSettings() override;

    static ReaderSettings *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static ReaderSettings *instance() { return s_instance; }

    int fontSizeStep() const { return m_fontSizeStep; }
    void setFontSizeStep(int step);
    int fontSizeSteps() const;
    int fontPixelSize() const;
    QString theme() const { return m_theme; }
    QStringList themes() const;
    QString pageColor() const { return m_pageColor; }
    void setPageColor(const QString &color);
    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &family);
    QStringList fontFamilies() const;
    bool bold() const { return m_bold; }
    void setBold(bool bold);
    double lineHeight() const { return m_lineHeight; }
    void setLineHeight(double lineHeight);
    double letterSpacing() const { return m_letterSpacing; }
    void setLetterSpacing(double spacing);
    double wordSpacing() const { return m_wordSpacing; }
    void setWordSpacing(double spacing);
    int margin() const { return m_margin; }
    void setMargin(int margin);
    bool justify() const { return m_justify; }
    void setJustify(bool justify);
    bool twoPages() const { return m_twoPages; }
    void setTwoPages(bool twoPages);

    Q_INVOKABLE void smaller() { setFontSizeStep(m_fontSizeStep - 1); }
    Q_INVOKABLE void larger() { setFontSizeStep(m_fontSizeStep + 1); }
    Q_INVOKABLE void applyTheme(const QString &id);
    Q_INVOKABLE QVariantMap themeInfo(const QString &id) const;
    Q_INVOKABLE void reset();

    // The settings as the layout uses them; the colors are not part of it.
    TextStyle textStyle() const;

signals:
    void changed();

private:
    // A value was changed by hand: no theme any more; store and tell.
    void changedByHand();
    void store();

    QQmlEngine *m_engine = nullptr;
    bool m_applying = false;
    int m_fontSizeStep = 2;
    QString m_theme;
    QString m_pageColor;
    QString m_fontFamily;
    bool m_bold = false;
    double m_lineHeight = 1.6;
    double m_letterSpacing = 0.0;
    double m_wordSpacing = 0.0;
    int m_margin = 56;
    bool m_justify = true;
    bool m_twoPages = false;

    static inline ReaderSettings *s_instance = nullptr;
};
