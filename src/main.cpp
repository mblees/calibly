// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTimer>

#include "appearancesettings.h"
#include "exportcontroller.h"
#include "importcontroller.h"
#include "languagemanager.h"
#include "appinfo.h"
#include "librarycontroller.h"
#include "mediacontrols.h"
#include "librarylock.h"
#include "logfile.h"
#include "mockbackend.h"
#include "playercontroller.h"
#include "readercontroller.h"
#include "readersettings.h"
#include "sqlitebackend.h"
#include "updatechecker.h"

#include <QFileInfo>
#include <QSettings>
#include <memory>

// Development options (--mock, --gallery, --screenshot, --size) and the
// sample library exist only in builds with CALIBLY_DEV_TOOLS (CMake option,
// off in the release preset).
#ifdef CALIBLY_DEV_TOOLS
constexpr bool kDevTools = true;
#else
constexpr bool kDevTools = false;
#endif

// The mouse wheel scrolls the lists and grids (Qt Quick Flickable) by
// wheelScrollLines × 8 px per notch: with the 3 lines of Windows only 24 px,
// which feels slow for a cover gallery. calibly scrolls this many times the
// lines set in Windows.
constexpr int kWheelScrollFactor = 4;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("calibly"));
    QGuiApplication::setApplicationName(QStringLiteral("calibly"));
    QGuiApplication::setApplicationVersion(QStringLiteral(CALIBLY_VERSION));
    // Messages also go to %LOCALAPPDATA%\calibly\logs, for bug reports.
    LogFile::install();
    QGuiApplication::styleHints()->setWheelScrollLines(QGuiApplication::styleHints()->wheelScrollLines()
                                                       * kWheelScrollFactor);
    qInfo("calibly %s started", CALIBLY_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("One library for ebooks and audiobooks."));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption galleryOption(
        QStringLiteral("gallery"),
        QStringLiteral("Show the component gallery instead of the library (development)."));
    const QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Save a screenshot of the window to <file> and quit (development)."),
        QStringLiteral("file"));
    const QCommandLineOption languageOption(
        QStringLiteral("language"),
        QStringLiteral("Start with this UI language, e.g. \"de\" or \"en\"."),
        QStringLiteral("code"));
    const QCommandLineOption sizeOption(
        QStringLiteral("size"),
        QStringLiteral("Initial window size, e.g. \"1280x2400\" (development, for screenshots)."),
        QStringLiteral("WxH"));
    const QCommandLineOption mockOption(
        QStringLiteral("mock"),
        QStringLiteral("Read the library from this JSON file instead of the built-in sample data (development)."),
        QStringLiteral("file"));
    const QCommandLineOption libraryOption(
        QStringLiteral("library"),
        QStringLiteral("Use this library folder (default: the last used one, or Documents/calibly-Bibliothek)."),
        QStringLiteral("folder"));
    const QCommandLineOption importOption(
        QStringLiteral("import"),
        QStringLiteral("Import this file or folder into the library after start; can be given several times."),
        QStringLiteral("path"));
    parser.addOption(libraryOption);
    parser.addOption(importOption);
    parser.addOption(languageOption);
    if (kDevTools) {
        parser.addOption(mockOption);
        parser.addOption(galleryOption);
        parser.addOption(screenshotOption);
        parser.addOption(sizeOption);
    }
    parser.process(app);
    // Not defined without the development tools: isSet() would warn.
    const auto devOptionSet = [&parser](const QCommandLineOption &option) {
        return kDevTools && parser.isSet(option);
    };

    // Our controls are built on QtQuick.Templates. "Basic" keeps the few
    // standard controls we use (e.g. ScrollBar) identical on every platform.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Sample data with --mock. Created before the engine, so it outlives
    // everything that uses it.
    std::unique_ptr<LibraryBackend> mockBackend;
    if (devOptionSet(mockOption))
        mockBackend = std::make_unique<MockBackend>(parser.value(mockOption));

    QQmlApplicationEngine engine;

    // The controllers are QML singletons. They are children of the engine, so
    // they are destroyed only after the engine has deleted all QML objects.
    // The language must be set up before QML is loaded, so the first frame is
    // already translated.
    auto *languageManager = new LanguageManager(&engine, &engine);
    languageManager->start(parser.value(languageOption));
    auto *libraryController = mockBackend ? new LibraryController(*mockBackend, &engine, &engine)
                                          : new LibraryController(&engine, &engine);
    if (!mockBackend) {
        // The real library: the one given with --library, the last used one,
        // or a new one in Documents. A library that was used before is not
        // created again if it is gone; the window then says what happened.
        QSettings settings;
        const bool given = parser.isSet(libraryOption);
        const bool known = settings.contains(QStringLiteral("library/path"));
        const QString root = given ? parser.value(libraryOption)
                                   : settings.value(QStringLiteral("library/path"), SqliteBackend::defaultLibraryRoot()).toString();
        libraryController->setRememberLibrary(!given);
        if (libraryController->openLibrary(root, given || !known) == LibraryController::InUse) {
            // calibly already runs with this library: show that window instead.
            if (LibraryLock::activateOwner(root))
                return EXIT_SUCCESS;
        }
    }
    auto *importController = new ImportController(*libraryController, &engine, &engine);
    // Created after the library: it loads the last audiobook from it.
    auto *playerController = new PlayerController(*libraryController, &engine, &engine);
    auto *readerSettings = new ReaderSettings(&engine, &engine);
    new ReaderController(*libraryController, *readerSettings, &engine, &engine);
    new AppInfo(&engine, &engine);
    new AppearanceSettings(&engine, &engine);
    new ExportController(*libraryController, &engine, &engine);
    // Looks for a newer version once a day (not with sample data, which is
    // only used during development).
    auto *updateChecker = new UpdateChecker(QStringLiteral(CALIBLY_VERSION), &engine, &engine);
    if (!mockBackend)
        QTimer::singleShot(3000, updateChecker, &UpdateChecker::checkIfDue);

    engine.setInitialProperties({
        { QStringLiteral("galleryMode"), devOptionSet(galleryOption) },
    });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);

    engine.loadFromModule("Calibly", "Main");

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window)
        return EXIT_FAILURE;

    // The media keys and the media overlay of Windows. Not with sample data:
    // their files do not exist.
    if (!mockBackend) {
        // A child of the window: gone before the controllers it uses.
        auto *mediaControls = new MediaControls(*playerController, *libraryController, window);
        mediaControls->attach(window);
    }

    if (parser.isSet(importOption)) {
        QList<QUrl> urls;
        for (const QString &path : parser.values(importOption))
            urls.append(QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath()));
        importController->importFiles(urls);
    }

    if (devOptionSet(sizeOption)) {
        const QStringList parts = parser.value(sizeOption).split(u'x');
        if (parts.size() == 2)
            window->resize(parts.at(0).toInt(), parts.at(1).toInt());
    }

    if (devOptionSet(screenshotOption)) {
        const QString file = parser.value(screenshotOption);
        // Give the scene a moment to render its first frames.
        QTimer::singleShot(1500, window, [window, file] {
            const bool saved = window->grabWindow().save(file);
            QCoreApplication::exit(saved ? EXIT_SUCCESS : EXIT_FAILURE);
        });
    }

    return app.exec();
}
