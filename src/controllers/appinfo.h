#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

// Facts about calibly itself for "Über calibly" in the settings (release
// step 4). The license files lie next to calibly.exe in an installed
// calibly; in a development build they are not there and the URLs are empty.
//
// In QML (import Calibly.Controllers):
//   AppInfo.version          e.g. "0.1.0-beta"
//   AppInfo.copyright        e.g. "Copyright (C) 2026 mblees"
//   AppInfo.releasesUrl      the download page
//   AppInfo.sourceUrl        the source code (calibly is GPL 3)
//   AppInfo.licenseUrl       LICENSE.txt (the GNU GPL 3)
//   AppInfo.noticesUrl       THIRD-PARTY-NOTICES.txt
//   AppInfo.logFolderUrl     the folder of the log files
//   AppInfo.folderUrl(path)  a local folder or file as URL, e.g. for
//                            Qt.openUrlExternally()
class AppInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString copyright READ copyright CONSTANT)
    Q_PROPERTY(QUrl releasesUrl READ releasesUrl CONSTANT)
    Q_PROPERTY(QUrl sourceUrl READ sourceUrl CONSTANT)
    Q_PROPERTY(QUrl licenseUrl READ licenseUrl CONSTANT)
    Q_PROPERTY(QUrl noticesUrl READ noticesUrl CONSTANT)
    Q_PROPERTY(QUrl logFolderUrl READ logFolderUrl CONSTANT)

public:
    explicit AppInfo(QQmlEngine *engine, QObject *parent = nullptr);
    ~AppInfo() override;

    static AppInfo *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QString version() const;
    QString copyright() const;
    QUrl releasesUrl() const;
    QUrl sourceUrl() const;
    QUrl licenseUrl() const;
    QUrl noticesUrl() const;
    QUrl logFolderUrl() const;
    Q_INVOKABLE QUrl folderUrl(const QString &path) const;

private:
    QUrl besideProgram(const QString &fileName) const;

    QQmlEngine *m_engine = nullptr;

    static inline AppInfo *s_instance = nullptr;
};
