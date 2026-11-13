// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDateTime>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

// Looks for a newer calibly on the releases page (release step 4): asks
// GitHub's API for the releases of the repository CALIBLY_RELEASES_REPO
// (it must be public for this) – no account, no personal data. Pre-releases
// count while a pre-release (e.g. "0.1.0-beta") is installed.
//
// In QML (import Calibly.Controllers):
//   UpdateChecker.enabled        check at start, at most once a day (setting, on by default)
//   UpdateChecker.state          Idle, Checking, UpToDate, Available, Failed
//   UpdateChecker.latestVersion  e.g. "0.2.0-beta" (Available)
//   UpdateChecker.releaseUrl     its page, to download it
//   UpdateChecker.lastChecked    when the last check succeeded, invalid = never
//   UpdateChecker.hintVisible    Available and not dismissed: show the hint
//   UpdateChecker.check()        check now (button "Jetzt suchen")
//   UpdateChecker.checkIfDue()   check if enabled and the last check is a day ago
//   UpdateChecker.dismiss()      "Ignorieren": no hint for this version again
class UpdateChecker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(State state READ state NOTIFY changed)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(QUrl releaseUrl READ releaseUrl NOTIFY changed)
    Q_PROPERTY(QDateTime lastChecked READ lastChecked NOTIFY changed)
    Q_PROPERTY(bool hintVisible READ hintVisible NOTIFY changed)

public:
    enum State { Idle, Checking, UpToDate, Available, Failed };
    Q_ENUM(State)

    struct Release
    {
        QString version;
        QUrl url;
    };

    UpdateChecker(const QString &currentVersion, QQmlEngine *engine, QObject *parent = nullptr);
    ~UpdateChecker() override;

    static UpdateChecker *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    bool enabled() const;
    void setEnabled(bool enabled);
    State state() const { return m_state; }
    QString currentVersion() const { return m_currentVersion; }
    QString latestVersion() const { return m_latest.version; }
    QUrl releaseUrl() const { return m_latest.url; }
    QDateTime lastChecked() const;
    bool hintVisible() const;

    Q_INVOKABLE void check();
    Q_INVOKABLE void checkIfDue();
    Q_INVOKABLE void dismiss();

    // The newest release in GitHub's answer (JSON list of releases) that is
    // newer than current; drafts are left out, pre-releases only count when
    // current is one. Empty version = none.
    static Release newestRelease(const QByteArray &json, const QString &current);
    // < 0, 0, > 0 like strcmp: "1.2.0" > "1.2.0-beta" > "1.1.9"; a leading "v" is ignored.
    static int compareVersions(const QString &a, const QString &b);
    // Takes GitHub's answer (also used by the tests).
    void handleAnswer(const QByteArray &json, const QDateTime &now = QDateTime::currentDateTime());

signals:
    void changed();

private:
    void setState(State state);

    QString m_currentVersion;
    QQmlEngine *m_engine = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    State m_state = Idle;
    Release m_latest;

    static inline UpdateChecker *s_instance = nullptr;
};
