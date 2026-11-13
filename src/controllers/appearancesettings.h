// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QColor>
#include <QObject>
#include <QQmlEngine>

// How calibly looks: the accent color, chosen in the settings with a color
// wheel. The accent is also the color of listening (audiobooks), so both
// change together; reading stays blue. Kept between starts.
//
// In QML (import Calibly.Controllers):
//   AppearanceSettings.accentColor        the accent; setting it stores it
//   AppearanceSettings.accentForeground   text and icons on the accent: a
//                                         dark shade of it, or white on dark
//                                         accents
//   AppearanceSettings.defaultAccentColor calibly's orange
//   AppearanceSettings.isDefault          the accent is calibly's orange
//   AppearanceSettings.minimumBrightness  darker colors (HSV value) are
//                                         raised to it, so the accent stays
//                                         visible on the dark background
//   AppearanceSettings.reset()            back to orange
// Main.qml hands accentColor and accentForeground to Theme.
class AppearanceSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QColor accentForeground READ accentForeground NOTIFY accentColorChanged)
    Q_PROPERTY(QColor defaultAccentColor READ defaultAccentColor CONSTANT)
    Q_PROPERTY(bool isDefault READ isDefault NOTIFY accentColorChanged)
    Q_PROPERTY(double minimumBrightness READ minimumBrightness CONSTANT)

public:
    explicit AppearanceSettings(QQmlEngine *engine, QObject *parent = nullptr);
    ~AppearanceSettings() override;

    static AppearanceSettings *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QColor accentColor() const { return m_accent; }
    void setAccentColor(const QColor &color);
    QColor accentForeground() const { return foregroundFor(m_accent); }
    static QColor defaultAccentColor();
    bool isDefault() const { return m_accent == defaultAccentColor(); }
    static double minimumBrightness();

    Q_INVOKABLE void reset();

    // The color as an accent: opaque, at least minimumBrightness bright.
    // Invalid colors give the default.
    static QColor usableAccent(const QColor &color);
    // Text color on background: a very dark shade of it, or white if that
    // contrasts better.
    static QColor foregroundFor(const QColor &background);

signals:
    void accentColorChanged();

private:
    static AppearanceSettings *s_instance;

    QQmlEngine *m_engine = nullptr;
    QColor m_accent;
};
