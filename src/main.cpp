#include <QCommandLineParser>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

#include "languagemanager.h"
#include "librarycontroller.h"
#include "mockbackend.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("calibly"));
    QGuiApplication::setApplicationName(QStringLiteral("calibly"));
    QGuiApplication::setApplicationVersion(QStringLiteral(CALIBLY_VERSION));

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
    parser.addOption(galleryOption);
    parser.addOption(screenshotOption);
    parser.addOption(languageOption);
    parser.addOption(sizeOption);
    parser.process(app);

    // Our controls are built on QtQuick.Templates. "Basic" keeps the few
    // standard controls we use (e.g. ScrollBar) identical on every platform.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // Phase 1: sample data. Phase 4 replaces this with the real library.
    // Declared before the engine, so it outlives everything that uses it.
    MockBackend backend;

    QQmlApplicationEngine engine;

    // The controllers are QML singletons. They are children of the engine, so
    // they are destroyed only after the engine has deleted all QML objects.
    // The language must be set up before QML is loaded, so the first frame is
    // already translated.
    auto *languageManager = new LanguageManager(&engine, &engine);
    languageManager->start(parser.value(languageOption));
    new LibraryController(backend, &engine, &engine);

    engine.setInitialProperties({
        { QStringLiteral("galleryMode"), parser.isSet(galleryOption) },
    });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);

    engine.loadFromModule("Calibly", "Main");

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window)
        return EXIT_FAILURE;

    if (parser.isSet(sizeOption)) {
        const QStringList parts = parser.value(sizeOption).split(u'x');
        if (parts.size() == 2)
            window->resize(parts.at(0).toInt(), parts.at(1).toInt());
    }

    if (parser.isSet(screenshotOption)) {
        const QString file = parser.value(screenshotOption);
        // Give the scene a moment to render its first frames.
        QTimer::singleShot(1500, window, [window, file] {
            const bool saved = window->grabWindow().save(file);
            QCoreApplication::exit(saved ? EXIT_SUCCESS : EXIT_FAILURE);
        });
    }

    return app.exec();
}
