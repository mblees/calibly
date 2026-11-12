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
//   LanguageManager.followsSystem   no language was chosen: the system's is used
//   LanguageManager.followSystem()  forget the chosen language and use the
//                                   system's (now and at the next start)
//   LanguageManager.chooseLanguage(code)  use this language from now on, also
//                                   if it is the system's (settings page)
class LanguageManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList languages READ languages CONSTANT)
    Q_PROPERTY(QString sourceLanguage READ sourceLanguage CONSTANT)
    Q_PROPERTY(bool followsSystem READ followsSystem NOTIFY followsSystemChanged)

public:
    explicit LanguageManager(QQmlEngine *engine, QObject *parent = nullptr);
    ~LanguageManager() override;

    // Used by the QML engine to obtain the singleton instance created in main().
    static LanguageManager *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QVariantList languages() const;
    QString sourceLanguage() const;
    QStringList languageCodes() const;
    bool followsSystem() const;
    Q_INVOKABLE void followSystem();
    Q_INVOKABLE void chooseLanguage(const QString &code);

    // Sets the start language and loads its translation. Order of preference:
    // requested (command line) > saved choice > system language > source language.
    void start(const QString &requested = QString());

signals:
    void followsSystemChanged();

private:
    void applyLanguage();
    QString pickStartLanguage(const QString &requested) const;
    // The first available language of the system, else the source language.
    QString systemLanguage() const;

    QQmlEngine *m_engine = nullptr;
    QStringList m_codes;
    QString m_applied;
    QTranslator m_translator;
    bool m_followingSystem = false; // a language set by followSystem() is not stored

    static inline LanguageManager *s_instance = nullptr;
};
