#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QTranslator>
#include <QVariantList>

// Switches the UI language at runtime using Qt's own mechanisms:
//
//   * The current language is QQmlEngine::uiLanguage, in QML: Qt.uiLanguage.
//     Changing the language from QML is simply:   Qt.uiLanguage = "en"
//   * On every change the matching translation (:/i18n/calibly_<code>.qm,
//     including Qt's own texts) is installed and QQmlEngine::retranslate()
//     updates all qsTr() texts on screen.
//   * The choice is stored with QSettings and used on the next start.
//
// In QML (import Calibly.Controllers):
//   LanguageManager.languages       list of { code, name }, e.g. { "en", "English" }
//   LanguageManager.sourceLanguage  language of the texts in the source code ("de")
class LanguageManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList languages READ languages CONSTANT)
    Q_PROPERTY(QString sourceLanguage READ sourceLanguage CONSTANT)

public:
    explicit LanguageManager(QQmlEngine *engine, QObject *parent = nullptr);
    ~LanguageManager() override;

    // Used by the QML engine to obtain the singleton instance created in main().
    static LanguageManager *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QVariantList languages() const;
    QString sourceLanguage() const;
    QStringList languageCodes() const;

    // Sets the start language and loads its translation. Order of preference:
    // requested (command line) > saved choice > system language > source language.
    void start(const QString &requested = QString());

private:
    void applyLanguage();
    QString pickStartLanguage(const QString &requested) const;

    QQmlEngine *m_engine = nullptr;
    QStringList m_codes;
    QString m_applied;
    QTranslator m_translator;

    static inline LanguageManager *s_instance = nullptr;
};
