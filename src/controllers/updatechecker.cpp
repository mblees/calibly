// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "updatechecker.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>

#ifndef CALIBLY_RELEASES_REPO
#define CALIBLY_RELEASES_REPO "mblees/calibly"
#endif

namespace {

const QString kEnabledKey = QStringLiteral("updates/check");
const QString kLastCheckKey = QStringLiteral("updates/lastCheck");
const QString kDismissedKey = QStringLiteral("updates/dismissed");

struct Version
{
    QList<int> numbers;
    QString suffix; // "" = final release
};

Version parse(QString text)
{
    text = text.trimmed();
    if (text.startsWith(u'v') || text.startsWith(u'V'))
        text.remove(0, 1);
    const qsizetype dash = text.indexOf(u'-');
    Version version;
    version.suffix = dash >= 0 ? text.mid(dash + 1) : QString();
    for (const QString &part : text.left(dash >= 0 ? dash : text.size()).split(u'.'))
        version.numbers.append(part.toInt());
    while (version.numbers.size() < 3)
        version.numbers.append(0);
    return version;
}

bool isPreRelease(const QString &version)
{
    return !parse(version).suffix.isEmpty();
}

} // namespace

UpdateChecker::UpdateChecker(const QString &currentVersion, QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_currentVersion(currentVersion)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

UpdateChecker::~UpdateChecker()
{
    s_instance = nullptr;
}

UpdateChecker *UpdateChecker::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

bool UpdateChecker::enabled() const
{
    return QSettings().value(kEnabledKey, true).toBool();
}

void UpdateChecker::setEnabled(bool enabled)
{
    if (enabled == this->enabled())
        return;
    QSettings().setValue(kEnabledKey, enabled);
    emit changed();
}

QDateTime UpdateChecker::lastChecked() const
{
    return QSettings().value(kLastCheckKey).toDateTime();
}

bool UpdateChecker::hintVisible() const
{
    return m_state == Available && QSettings().value(kDismissedKey).toString() != m_latest.version;
}

void UpdateChecker::setState(State state)
{
    m_state = state;
    emit changed();
}

void UpdateChecker::checkIfDue()
{
    const QDateTime last = lastChecked();
    if (enabled() && (!last.isValid() || last.secsTo(QDateTime::currentDateTime()) >= 24 * 60 * 60))
        check();
}

void UpdateChecker::check()
{
    if (m_reply)
        return;
    if (!m_network)
        m_network = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com/repos/%1/releases?per_page=20")
                                     .arg(QStringLiteral(CALIBLY_RELEASES_REPO))));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("calibly/%1").arg(m_currentVersion));
    request.setTransferTimeout(15000);
    m_reply = m_network->get(request);
    setState(Checking);
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            // No connection is no reason to bother the user: only the
            // settings page shows it.
            qInfo("Update check failed: %s", qPrintable(reply->errorString()));
            setState(Failed);
            return;
        }
        handleAnswer(reply->readAll());
    });
}

void UpdateChecker::handleAnswer(const QByteArray &json, const QDateTime &now)
{
    QJsonParseError error;
    QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError) {
        setState(Failed);
        return;
    }
    QSettings().setValue(kLastCheckKey, now);
    m_latest = newestRelease(json, m_currentVersion);
    setState(m_latest.version.isEmpty() ? UpToDate : Available);
}

void UpdateChecker::dismiss()
{
    QSettings().setValue(kDismissedKey, m_latest.version);
    emit changed();
}

UpdateChecker::Release UpdateChecker::newestRelease(const QByteArray &json, const QString &current)
{
    const bool wantPreReleases = isPreRelease(current);
    Release newest;
    const QJsonArray releases = QJsonDocument::fromJson(json).array();
    for (const QJsonValue &value : releases) {
        const QJsonObject release = value.toObject();
        if (release.value(QStringLiteral("draft")).toBool())
            continue;
        if (release.value(QStringLiteral("prerelease")).toBool() && !wantPreReleases)
            continue;
        QString version = release.value(QStringLiteral("tag_name")).toString();
        if (version.startsWith(u'v') || version.startsWith(u'V'))
            version.remove(0, 1);
        if (version.isEmpty() || compareVersions(version, current) <= 0)
            continue;
        if (newest.version.isEmpty() || compareVersions(version, newest.version) > 0)
            newest = { version, QUrl(release.value(QStringLiteral("html_url")).toString()) };
    }
    return newest;
}

int UpdateChecker::compareVersions(const QString &a, const QString &b)
{
    const Version x = parse(a);
    const Version y = parse(b);
    for (qsizetype i = 0; i < std::max(x.numbers.size(), y.numbers.size()); ++i) {
        const int left = x.numbers.value(i);
        const int right = y.numbers.value(i);
        if (left != right)
            return left < right ? -1 : 1;
    }
    // A final release is newer than its pre-releases.
    if (x.suffix.isEmpty() != y.suffix.isEmpty())
        return x.suffix.isEmpty() ? 1 : -1;
    // "beta.2" > "beta" > "alpha"; numbers in it compare as numbers.
    static const QRegularExpression parts(QStringLiteral("[.-]"));
    const QStringList left = x.suffix.split(parts);
    const QStringList right = y.suffix.split(parts);
    for (qsizetype i = 0; i < std::max(left.size(), right.size()); ++i) {
        if (i >= left.size())
            return -1;
        if (i >= right.size())
            return 1;
        bool leftNumber = false;
        bool rightNumber = false;
        const int l = left.at(i).toInt(&leftNumber);
        const int r = right.at(i).toInt(&rightNumber);
        const int result = leftNumber && rightNumber ? (l > r) - (l < r) : left.at(i).compare(right.at(i));
        if (result != 0)
            return result < 0 ? -1 : 1;
    }
    return 0;
}
