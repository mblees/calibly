// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "languagemanager.h"

#include "textutils.h"

#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QSettings>
#include <QVariantMap>

namespace {

const QString kSourceLanguage = QStringLiteral("de");
const QString kSettingsKey = QStringLiteral("ui/language");
const QString kTranslationDir = QStringLiteral(":/i18n");

} // namespace

LanguageManager::LanguageManager(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    // Available languages = source language + every embedded calibly_<code>.qm.
    m_codes.append(kSourceLanguage);
    const QStringList files = QDir(kTranslationDir).entryList({ QStringLiteral("calibly_*.qm") });
    for (const QString &file : files) {
        const QString code = file.section(u'_', 1).section(u'.', 0, 0);
        if (!m_codes.contains(code))
            m_codes.append(code);
    }

    connect(m_engine, &QJSEngine::uiLanguageChanged, this, &LanguageManager::applyLanguage);
}

LanguageManager::~LanguageManager()
{
    s_instance = nullptr;
}

LanguageManager *LanguageManager::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QVariantList LanguageManager::languages() const
{
    QVariantList list;
    for (const QString &code : m_codes)
        list.append(QVariantMap{ { QStringLiteral("code"), code },
                                 { QStringLiteral("name"), TextUtils::languageName(code) } });
    return list;
}

QString LanguageManager::sourceLanguage() const
{
    return kSourceLanguage;
}

QStringList LanguageManager::languageCodes() const
{
    return m_codes;
}

void LanguageManager::start(const QString &requested)
{
    if (!requested.isEmpty() && !m_codes.contains(requested))
        qWarning("Unknown language \"%s\", available: %s", qPrintable(requested),
                 qPrintable(m_codes.join(QStringLiteral(", "))));

    m_engine->setUiLanguage(pickStartLanguage(requested));
    applyLanguage(); // in case uiLanguage already had this value
}

QString LanguageManager::pickStartLanguage(const QString &requested) const
{
    if (m_codes.contains(requested))
        return requested;

    const QString saved = QSettings().value(kSettingsKey).toString();
    if (m_codes.contains(saved))
        return saved;

    return systemLanguage();
}

QString LanguageManager::systemLanguage() const
{
    const QStringList systemLanguages = QLocale::system().uiLanguages();
    for (const QString &tag : systemLanguages) {
        const QString code = QLocale::languageToCode(QLocale(tag).language());
        if (m_codes.contains(code))
            return code;
    }
    return kSourceLanguage;
}

bool LanguageManager::followsSystem() const
{
    return !QSettings().contains(kSettingsKey);
}

void LanguageManager::chooseLanguage(const QString &code)
{
    if (!m_codes.contains(code))
        return;
    QSettings().setValue(kSettingsKey, code);
    m_engine->setUiLanguage(code);
    emit followsSystemChanged();
}

void LanguageManager::followSystem()
{
    QSettings().remove(kSettingsKey);
    m_followingSystem = true;
    m_engine->setUiLanguage(systemLanguage());
    m_followingSystem = false;
    emit followsSystemChanged();
}

void LanguageManager::applyLanguage()
{
    QString code = m_engine->uiLanguage();
    if (!m_codes.contains(code)) {
        qWarning("Language \"%s\" is not available, using \"%s\"", qPrintable(code),
                 qPrintable(kSourceLanguage));
        code = kSourceLanguage;
    }
    if (code == m_applied)
        return;

    const bool firstStart = m_applied.isEmpty();

    QCoreApplication::removeTranslator(&m_translator);
    // The source language has no translation of its own, but its .qm file
    // carries plural forms and Qt's own texts (e.g. standard dialogs).
    if (m_translator.load(QStringLiteral("%1/calibly_%2.qm").arg(kTranslationDir, code)))
        QCoreApplication::installTranslator(&m_translator);

    // Numbers and dates follow the UI language.
    QLocale::setDefault(QLocale(code));

    m_applied = code;
    if (!firstStart && !m_followingSystem) {
        QSettings().setValue(kSettingsKey, code);
        emit followsSystemChanged();
    }

    m_engine->retranslate();
}
