// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QCoreApplication>
#include <QFile>
#include <QImage>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWindow>

#include <memory>

#include "librarybackend.h"
#include "librarycontroller.h"
#include "mediacontrols.h"
#include "playercontroller.h"

namespace {

// MP3 of silence (MPEG-1 Layer III, 128 kbit/s, 44.1 kHz, mono), no tags.
bool writeMp3(const QString &path, int durationMs)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QByteArray frame(417, '\0');
    frame[0] = char(0xFF);
    frame[1] = char(0xFB);
    frame[2] = char(0x90);
    frame[3] = char(0xC4);
    const int frames = int(durationMs / (1152.0 / 44.1));
    for (int i = 0; i < frames; ++i)
        file.write(frame);
    return true;
}

class MemoryBackend : public LibraryBackend
{
public:
    QList<Work> works;
    QString root;

    QString libraryRoot() const override { return root; }
    QList<Work> loadWorks() override { return works; }
    bool saveWork(const Work &work) override
    {
        for (Work &existing : works) {
            if (existing.id == work.id)
                existing = work;
        }
        return true;
    }
};

} // namespace

class TestMediaControls : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Own settings, so the tests neither see nor change the app's last audiobook.
        QCoreApplication::setOrganizationName(QStringLiteral("calibly-tests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_mediacontrols"));
        QVERIFY(m_dir.isValid());
        QVERIFY(writeMp3(m_dir.filePath(QStringLiteral("book.mp3")), 2000));
        QImage cover(300, 300, QImage::Format_RGB32);
        cover.fill(Qt::darkCyan);
        QVERIFY(cover.save(m_dir.filePath(QStringLiteral("book.mp3.cover.jpg"))));
    }

    void init()
    {
        QSettings().clear();
        m_backend = std::make_unique<MemoryBackend>();
        m_backend->root = m_dir.path();
        Work work;
        work.id = QStringLiteral("marsianer");
        work.title = QStringLiteral("Der Marsianer");
        work.authors = { QStringLiteral("Andy Weir") };
        Format audiobook;
        audiobook.kind = Format::Kind::Audiobook;
        audiobook.filePath = m_dir.filePath(QStringLiteral("book.mp3"));
        audiobook.coverPath = m_dir.filePath(QStringLiteral("book.mp3.cover.jpg"));
        audiobook.coverIsSquare = true;
        // Longer than the file, so skipping has room; nothing plays that long.
        audiobook.durationSeconds = 240;
        audiobook.chapters = { { QStringLiteral("Prolog"), QString(), 0, 120000 },
                               { QString(), QString(), 120000, 120000 } };
        work.formats = { audiobook };
        m_backend->works = { work };
        m_library = std::make_unique<LibraryController>(*m_backend, nullptr);
        m_player = std::make_unique<PlayerController>(*m_library, nullptr);
    }

    void cleanup()
    {
        m_player.reset();
        m_library.reset();
        m_backend.reset();
    }

    // The audiobook loaded at start does not take over the media keys.
    void shownOnlyAfterPlaying()
    {
        MediaControls controls(*m_player, *m_library);
        QVERIFY(m_player->load(QStringLiteral("marsianer")));
        QVERIFY(!controls.display().enabled);

        QSignalSpy changed(&controls, &MediaControls::displayChanged);
        QVERIFY(m_player->play(QStringLiteral("marsianer")));
        MediaControls::Display display = controls.display();
        QVERIFY(display.enabled);
        QCOMPARE(display.status, MediaControls::Status::Playing);
        QCOMPARE(display.title, QStringLiteral("Der Marsianer"));
        QCOMPARE(display.artist, QStringLiteral("Andy Weir"));
        QCOMPARE(display.album, QStringLiteral("Prolog"));
        QCOMPARE(display.coverPath, m_dir.filePath(QStringLiteral("book.mp3.cover.jpg")));
        QVERIFY(changed.count() >= 1);

        m_player->pause();
        QCOMPARE(controls.display().status, MediaControls::Status::Paused);
        QVERIFY(controls.display().enabled); // stays while there is an audiobook

        // An untitled chapter is numbered.
        m_player->seek(130000);
        QCOMPARE(controls.display().album, QStringLiteral("Kapitel 2"));

        // A new title reaches the overlay.
        Work renamed = *m_library->works()->workAt(0);
        renamed.title = QStringLiteral("The Martian");
        QVERIFY(m_library->saveWork(renamed));
        QCOMPARE(controls.display().title, QStringLiteral("The Martian"));

        m_player->close();
        QVERIFY(!controls.display().enabled);
        QCOMPARE(controls.display().status, MediaControls::Status::Closed);
    }

    void buttons()
    {
        MediaControls controls(*m_player, *m_library);
        controls.handleButton(MediaControls::Button::Play); // no audiobook: nothing
        QVERIFY(!m_player->hasWork());

        QVERIFY(m_player->play(QStringLiteral("marsianer")));
        controls.handleButton(MediaControls::Button::Pause);
        QVERIFY(!m_player->playing());
        controls.handleButton(MediaControls::Button::Play);
        QVERIFY(m_player->playing());
        controls.handleButton(MediaControls::Button::Stop);
        QVERIFY(!m_player->playing());
        QVERIFY(m_player->hasWork()); // stop only pauses

        m_player->seek(60000);
        controls.handleButton(MediaControls::Button::Next);
        QCOMPARE(m_player->position(), 90000.0);
        controls.handleButton(MediaControls::Button::FastForward);
        QCOMPARE(m_player->position(), 120000.0);
        controls.handleButton(MediaControls::Button::Previous);
        QCOMPARE(m_player->position(), 90000.0);
        controls.handleButton(MediaControls::Button::Rewind);
        QCOMPARE(m_player->position(), 60000.0);
        m_player->seek(10000);
        controls.handleButton(MediaControls::Button::Previous);
        QCOMPARE(m_player->position(), 0.0);
    }

    // The media controls of Windows accept the window and the cover. Nothing
    // is played, so calibly never appears in the overlay of the system.
    void attachesToWindow()
    {
#ifdef Q_OS_WIN
        QWindow window;
        window.create();
        MediaControls controls(*m_player, *m_library);
        QVERIFY(controls.attach(&window));
        QVERIFY(controls.attach(&window)); // twice is fine
        QVERIFY(MediaControls::createsThumbnail(m_dir.filePath(QStringLiteral("book.mp3.cover.jpg"))));
        QVERIFY(!MediaControls::createsThumbnail(m_dir.filePath(QStringLiteral("book.mp3"))));
        QVERIFY(!MediaControls::createsThumbnail(QString()));
#else
        QSKIP("Windows only");
#endif
    }

private:
    QTemporaryDir m_dir;
    std::unique_ptr<MemoryBackend> m_backend;
    std::unique_ptr<LibraryController> m_library;
    std::unique_ptr<PlayerController> m_player;
};

QTEST_MAIN(TestMediaControls)
#include "tst_mediacontrols.moc"
