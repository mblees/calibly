#include <QCommandLineParser>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

#include "languagemanager.h"

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
    parser.addOption(galleryOption);
    parser.addOption(screenshotOption);
    parser.addOption(languageOption);
    parser.process(app);

    // Our controls are built on QtQuick.Templates. "Basic" keeps the few
    // standard controls we use (e.g. ScrollBar) identical on every platform.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;

    // Must be set up before QML is loaded, so the first frame is already translated.
    LanguageManager languageManager(&engine);
    languageManager.start(parser.value(languageOption));

    engine.setInitialProperties({
        { QStringLiteral("galleryMode"), parser.isSet(galleryOption) },
    });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);

    engine.loadFromModule("Calibly", "Main");

    if (parser.isSet(screenshotOption)) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
        if (!window)
            return EXIT_FAILURE;
        const QString file = parser.value(screenshotOption);
        // Give the scene a moment to render its first frames.
        QTimer::singleShot(1500, window, [window, file] {
            const bool saved = window->grabWindow().save(file);
            QCoreApplication::exit(saved ? EXIT_SUCCESS : EXIT_FAILURE);
        });
    }

    return app.exec();
}
