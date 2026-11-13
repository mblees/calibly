// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QCoreApplication>
#include <QQmlEngine>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

#include "appearancesettings.h"
#include "languagemanager.h"
#include "updatechecker.h"

namespace {

// GitHub's answer for the releases of a repository, newest first.
const QByteArray kReleases =
    "["
    "{ \"tag_name\": \"v0.3.0\", \"draft\": true, \"prerelease\": false, \"html_url\": \"https://example.org/v0.3.0\" },"
    "{ \"tag_name\": \"v0.2.0-beta.2\", \"draft\": false, \"prerelease\": true, \"html_url\": \"https://example.org/v0.2.0-beta.2\" },"
    "{ \"tag_name\": \"v0.2.0-beta\", \"draft\": false, \"prerelease\": true, \"html_url\": \"https://example.org/v0.2.0-beta\" },"
    "{ \"tag_name\": \"v0.1.1\", \"draft\": false, \"prerelease\": false, \"html_url\": \"https://example.org/v0.1.1\" },"
    "{ \"tag_name\": \"v0.1.0-beta\", \"draft\": false, \"prerelease\": true, \"html_url\": \"https://example.org/v0.1.0-beta\" }"
    "]";

} // namespace

class TestAppSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Own settings, so the tests neither see nor change the app's.
        QCoreApplication::setOrganizationName(QStringLiteral("calibly-tests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_appsettings"));
        QSettings().clear();
    }

    void cleanupTestCase()
    {
        QSettings().clear();
    }

    void compareVersions_data()
    {
        QTest::addColumn<QString>("a");
        QTest::addColumn<QString>("b");
        QTest::addColumn<int>("result");
        QTest::newRow("same") << "0.1.0" << "0.1.0" << 0;
        QTest::newRow("leading v") << "v0.2.0" << "0.2.0" << 0;
        QTest::newRow("patch") << "0.1.1" << "0.1.0" << 1;
        QTest::newRow("minor beats patch") << "0.2.0" << "0.1.9" << 1;
        QTest::newRow("numbers, not text") << "0.10.0" << "0.9.0" << 1;
        QTest::newRow("release beats beta") << "1.0.0" << "1.0.0-beta" << 1;
        QTest::newRow("beta.2 beats beta") << "1.0.0-beta.2" << "1.0.0-beta" << 1;
        QTest::newRow("beta.10 beats beta.9") << "1.0.0-beta.10" << "1.0.0-beta.9" << 1;
        QTest::newRow("beta beats alpha") << "1.0.0-beta" << "1.0.0-alpha" << 1;
        QTest::newRow("short") << "1.0" << "1.0.0" << 0;
    }

    void compareVersions()
    {
        QFETCH(QString, a);
        QFETCH(QString, b);
        QFETCH(int, result);
        QCOMPARE(UpdateChecker::compareVersions(a, b), result);
        QCOMPARE(UpdateChecker::compareVersions(b, a), -result);
    }

    // The accent: orange by default, kept between starts, never too dark,
    // with a readable color on it.
    void accentColor()
    {
        QSettings().remove(QStringLiteral("appearance"));
        const QColor orange(QStringLiteral("#ff8a3d"));
        {
            QQmlEngine engine;
            AppearanceSettings settings(&engine);
            QCOMPARE(settings.accentColor(), orange);
            QVERIFY(settings.isDefault());
            // The default foreground is close to the one Theme had: dark.
            QVERIFY(settings.accentForeground().lightnessF() < 0.1);

            QSignalSpy changed(&settings, &AppearanceSettings::accentColorChanged);
            settings.setAccentColor(QColor(QStringLiteral("#a77bf0")));
            QCOMPARE(changed.count(), 1);
            settings.setAccentColor(QColor(QStringLiteral("#a77bf0")));
            QCOMPARE(changed.count(), 1); // the same color is no change
            QVERIFY(!settings.isDefault());
        }
        {
            QQmlEngine engine;
            AppearanceSettings settings(&engine);
            QCOMPARE(settings.accentColor(), QColor(QStringLiteral("#a77bf0")));

            // Too dark: raised to the minimum brightness, hue kept.
            settings.setAccentColor(QColor(QStringLiteral("#100820")));
            QVERIFY(std::abs(settings.accentColor().valueF() - AppearanceSettings::minimumBrightness()) < 0.01);
            QVERIFY(std::abs(settings.accentColor().hsvHueF() - QColor(QStringLiteral("#100820")).hsvHueF()) < 0.01);
            // Dark accents get white text, light ones a dark shade.
            QCOMPARE(settings.accentForeground(), QColor(Qt::white));
            QVERIFY(AppearanceSettings::foregroundFor(QColor(QStringLiteral("#ffe08a"))).lightnessF() < 0.1);
            // Transparency and invalid colors are not accents.
            settings.setAccentColor(QColor(255, 0, 0, 40));
            QCOMPARE(settings.accentColor().alpha(), 255);
            QCOMPARE(AppearanceSettings::usableAccent(QColor()), orange);

            settings.reset();
            QVERIFY(settings.isDefault());
            QVERIFY(!QSettings().contains(QStringLiteral("appearance/accentColor")));
        }
    }

    // Pre-releases count only while one is installed; drafts never.
    void newestRelease()
    {
        UpdateChecker::Release release = UpdateChecker::newestRelease(kReleases, QStringLiteral("0.1.0-beta"));
        QCOMPARE(release.version, QStringLiteral("0.2.0-beta.2"));
        QCOMPARE(release.url, QUrl(QStringLiteral("https://example.org/v0.2.0-beta.2")));

        release = UpdateChecker::newestRelease(kReleases, QStringLiteral("0.1.0"));
        QCOMPARE(release.version, QStringLiteral("0.1.1"));

        release = UpdateChecker::newestRelease(kReleases, QStringLiteral("0.2.0-beta.2"));
        QVERIFY(release.version.isEmpty()); // nothing newer
        QVERIFY(UpdateChecker::newestRelease("not json", QStringLiteral("0.1.0")).version.isEmpty());
    }

    // An answer shows the hint until it is dismissed for that version.
    void hint()
    {
        QQmlEngine engine;
        UpdateChecker checker(QStringLiteral("0.1.0-beta"), &engine);
        QVERIFY(checker.enabled()); // on by default
        QCOMPARE(checker.state(), UpdateChecker::Idle);
        QSignalSpy changed(&checker, &UpdateChecker::changed);

        const QDateTime now(QDate(2026, 11, 12), QTime(9, 0));
        checker.handleAnswer(kReleases, now);
        QCOMPARE(checker.state(), UpdateChecker::Available);
        QCOMPARE(checker.latestVersion(), QStringLiteral("0.2.0-beta.2"));
        QVERIFY(checker.hintVisible());
        QCOMPARE(checker.lastChecked(), now);
        QVERIFY(changed.count() > 0);

        checker.dismiss();
        QVERIFY(!checker.hintVisible());
        QCOMPARE(checker.state(), UpdateChecker::Available); // the settings still show it

        checker.handleAnswer("[]", now);
        QCOMPARE(checker.state(), UpdateChecker::UpToDate);
        checker.handleAnswer("{ broken", now);
        QCOMPARE(checker.state(), UpdateChecker::Failed);

        checker.setEnabled(false);
        QVERIFY(!checker.enabled());
        QCOMPARE(QSettings().value(QStringLiteral("updates/check")).toBool(), false); // kept for the next start
        checker.setEnabled(true);
    }

    // The language follows the system until one is chosen, also when the
    // chosen one is the system's.
    void language()
    {
        QQmlEngine engine;
        LanguageManager languages(&engine);
        languages.start();
        QVERIFY(languages.followsSystem());
        const QString system = engine.uiLanguage();

        QSignalSpy follows(&languages, &LanguageManager::followsSystemChanged);
        languages.chooseLanguage(system);
        QVERIFY(!languages.followsSystem());
        QVERIFY(follows.count() > 0);

        // Other languages exist only where their translations are built in (the app).
        const QStringList codes = languages.languageCodes();
        if (codes.size() > 1) {
            const QString other = codes.at(0) == system ? codes.at(1) : codes.at(0);
            languages.chooseLanguage(other);
            QCOMPARE(engine.uiLanguage(), other);
        }
        languages.chooseLanguage(QStringLiteral("xx")); // unknown: ignored
        QVERIFY(!languages.followsSystem());

        languages.followSystem();
        QVERIFY(languages.followsSystem());
        QCOMPARE(engine.uiLanguage(), system);
    }
};

QTEST_MAIN(TestAppSettings)
#include "tst_appsettings.moc"
