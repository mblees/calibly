// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantList>

#include "audiotimeline.h"

class QAudioOutput;
class QMediaPlayer;
class LibraryController;

// Plays audiobooks (Qt Multimedia) and remembers the position per audiobook.
//
// All times are milliseconds on the time axis of the whole audiobook (see
// AudioTimeline): an audiobook of several files plays as one. The position
// is saved in the library (Format::positionMs and the listening progress)
// when pausing, when the chapter changes, every few seconds while playing
// and when the app quits. The last audiobook of a library is loaded again
// (paused) on the next start, so it resumes at the same position.
//
// In QML (import Calibly.Controllers):
//   PlayerController.play(workId)        load (resume where it was) and play
//   PlayerController.play(workId, 3)     play from the start of chapter 3
//   PlayerController.togglePlay(), skip(-30), seek(ms), playChapter(index)
//   PlayerController.position / duration / chapterIndex / chapters …
//
// Properties:
//   hasWork        an audiobook is loaded (the player bar is shown)
//   workId         its work
//   playing        playback runs (or is about to, while a file loads)
//   position, duration      ms on the whole audiobook
//   chapters       [{ title ("" = untitled), start, duration }] in ms
//   chapterIndex, chapterStart, chapterDuration   the current chapter
//   speed          playback rate (0.5 – 3), kept between starts
//   sleepMinutesLeft   minutes until the sleep timer pauses, 0 = off
//   sleepAtChapterEnd  the sleep timer pauses at the end of the chapter
//   error          PlayerController.NoError, FileMissing or PlaybackFailed
class PlayerController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool hasWork READ hasWork NOTIFY workChanged)
    Q_PROPERTY(QString workId READ workId NOTIFY workChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY workChanged)
    Q_PROPERTY(QVariantList chapters READ chapters NOTIFY workChanged)
    Q_PROPERTY(int chapterIndex READ chapterIndex NOTIFY chapterChanged)
    Q_PROPERTY(double chapterStart READ chapterStart NOTIFY chapterChanged)
    Q_PROPERTY(double chapterDuration READ chapterDuration NOTIFY chapterChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(int sleepMinutesLeft READ sleepMinutesLeft NOTIFY sleepTimerChanged)
    Q_PROPERTY(bool sleepAtChapterEnd READ sleepAtChapterEnd NOTIFY sleepTimerChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)

public:
    enum Error { NoError, FileMissing, PlaybackFailed };
    Q_ENUM(Error)

    PlayerController(LibraryController &library, QQmlEngine *engine, QObject *parent = nullptr);
    ~PlayerController() override;

    static PlayerController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    bool hasWork() const { return !m_workId.isEmpty(); }
    QString workId() const { return m_workId; }
    bool playing() const { return m_playing; }
    double position() const { return double(m_positionMs); }
    double duration() const { return double(m_timeline.durationMs()); }
    QVariantList chapters() const;
    int chapterIndex() const { return m_chapterIndex; }
    double chapterStart() const;
    double chapterDuration() const;
    double speed() const { return m_speed; }
    void setSpeed(double speed);
    int sleepMinutesLeft() const { return (m_sleepSecondsLeft + 59) / 60; }
    bool sleepAtChapterEnd() const { return m_sleepAtChapterEnd; }
    Error error() const { return m_error; }

    // Loads the audiobook of the work at its saved position and plays it
    // (from the start of a chapter if chapter >= 0). False if the work has no
    // audiobook or its file is missing.
    Q_INVOKABLE bool play(const QString &workId, int chapter = -1);
    // Loads without playing; used for the last audiobook on start.
    bool load(const QString &workId);

    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void seek(double positionMs);
    Q_INVOKABLE void skip(int seconds);
    Q_INVOKABLE void playChapter(int index);
    Q_INVOKABLE void nextChapter();
    // Back to the start of the chapter, or to the previous chapter within the
    // first seconds of a chapter (like a CD player).
    Q_INVOKABLE void previousChapter();
    // Removes the audiobook from the player (the position is kept).
    Q_INVOKABLE void close();

    // minutes > 0: pause after that many minutes of listening; 0 = off.
    Q_INVOKABLE void setSleepTimer(int minutes);
    Q_INVOKABLE void setSleepAtChapterEnd();

    // Stores the position in the library now.
    void savePosition();

signals:
    void workChanged();
    void playingChanged();
    void positionChanged();
    void chapterChanged();
    void speedChanged();
    void sleepTimerChanged();
    void errorChanged();

private:
    void setPlaying(bool playing);
    void setError(Error error);
    void setPositionMs(qint64 positionMs);
    // Makes the player play this part (file) at this time within the part.
    void loadPart(int part, qint64 offsetMs);
    void onMediaStatusChanged();
    void onPlayerPosition(qint64 partPositionMs);
    void onSleepTick();
    void unload();

    LibraryController &m_library;
    QQmlEngine *m_engine = nullptr;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_output = nullptr;

    QString m_workId;
    AudioTimeline m_timeline;
    int m_part = -1;               // part (file) loaded into m_player
    qint64 m_pendingOffsetMs = -1; // position to set once the part is loaded
    bool m_loadingSeen = false;    // the backend really loads the part (see loadPart())
    qint64 m_positionMs = 0;
    qint64 m_savedPositionMs = 0;  // the position the library knows (see savePosition())
    int m_chapterIndex = -1;
    bool m_playing = false;
    double m_speed = 1.0;
    Error m_error = NoError;

    QTimer m_saveTimer;
    QTimer m_sleepTimer;
    int m_sleepSecondsLeft = 0;
    bool m_sleepAtChapterEnd = false;

    static inline PlayerController *s_instance = nullptr;
};
