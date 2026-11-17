// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QString>

#include <memory>

class LibraryController;
class PlayerController;
class QWindow;

// The media keys and the media overlay of Windows (phase 14): the audiobook
// of the player appears in the overlay of the volume keys and on the lock
// screen with title, author, chapter and cover, and the media keys of the
// keyboard and of headphones play, pause and skip – also while calibly is in
// the background. Built on the SystemMediaTransportControls (SMTC) of the
// main window; on other systems attach() does nothing.
//
// The overlay shows calibly only once an audiobook was played since the
// start, so the paused audiobook loaded at start does not take the media
// keys from another app. It stays while the player has an audiobook.
//
// Buttons: play / pause (also play-pause and stop); next and previous skip
// 30 seconds forward / back, like → and ← in the app (an audiobook has no
// "tracks", and a double tap on headphones should not jump a chapter).
//
// Not a QML type: main.cpp creates it for the window.
class MediaControls : public QObject
{
    Q_OBJECT

public:
    enum class Status { Closed, Playing, Paused };
    enum class Button { Play, Pause, Stop, Next, Previous, FastForward, Rewind };

    // What the overlay shows.
    struct Display
    {
        bool enabled = false;   // calibly is in the overlay
        Status status = Status::Closed;
        QString title;          // the work
        QString artist;         // its authors
        QString album;          // the chapter
        QString coverPath;      // the audiobook's cover, "" = none

        bool operator==(const Display &other) const
        {
            return enabled == other.enabled && status == other.status && title == other.title
                && artist == other.artist && album == other.album && coverPath == other.coverPath;
        }
    };

    MediaControls(PlayerController &player, LibraryController &library, QObject *parent = nullptr);
    ~MediaControls() override;

    // Connects to the media controls of the system for this window; false
    // where there are none (other systems, or Windows refused).
    bool attach(QWindow *window);

    Display display() const;
    // A button of the overlay or a media key.
    void handleButton(Button button);

    // The cover can be handed to the overlay of the system (for the tests).
    static bool createsThumbnail(const QString &coverPath);

    // Skip of next / previous, in seconds.
    static constexpr int kSkipSeconds = 30;

signals:
    // display() changed.
    void displayChanged();

private:
    void update();

    class Platform;

    PlayerController &m_player;
    LibraryController &m_library;
    bool m_played = false;  // an audiobook was played since the start
    Display m_shown;
    std::unique_ptr<Platform> m_platform;
};
