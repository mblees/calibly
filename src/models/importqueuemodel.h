// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>
#include <QStringList>
#include <QUrl>

#include <functional>

// The import queue at the bottom right of the library (view D): one row per
// dropped batch while its files are read, then one row per work the batch
// becomes. Filled by ImportController; QML only shows it.
//
// In QML (ImportController.queue):
//   roles        entryId, title, status, newWork, fileTypes ("MP3", "EPUB + M4B"),
//                fileCount, merged, hasEbook, hasAudiobook, progress (0 – 1),
//                coverUrl, coverIsSquare, workId
//   count, finishedCount   all rows / rows that are no longer running
//   attentionCount         rows that need review or failed
//   active                 something is waiting, being read or copied
//   markReviewed(entryId)  a "needs review" row was looked at: it becomes done
//   clearFinished()        removes all rows that are no longer running
class ImportQueueModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by ImportController.queue")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int finishedCount READ finishedCount NOTIFY summaryChanged)
    Q_PROPERTY(int attentionCount READ attentionCount NOTIFY summaryChanged)
    Q_PROPERTY(bool active READ active NOTIFY summaryChanged)

public:
    enum Status {
        Waiting,      // dropped while another import runs
        Reading,      // the files are being read, the works are not known yet
        Copying,      // the files of this work are being copied
        Done,
        NeedsReview,  // imported, but the title is only the file name
        Failed,       // no file of this work could be copied
        Skipped,      // the files are already in the library
    };
    Q_ENUM(Status)

    enum Role {
        EntryIdRole = Qt::UserRole + 1,
        TitleRole,
        StatusRole,
        NewWorkRole,
        FileTypesRole,
        FileCountRole,
        MergedRole,
        HasEbookRole,
        HasAudiobookRole,
        ProgressRole,
        CoverUrlRole,
        CoverIsSquareRole,
        WorkIdRole,
    };
    Q_ENUM(Role)

    struct Entry
    {
        int id = 0;               // set by the model
        QString title;
        Status status = Waiting;
        bool newWork = true;
        QStringList fileTypes;
        int fileCount = 0;
        bool merged = false;
        bool hasEbook = false;
        bool hasAudiobook = false;
        double progress = 0.0;
        QUrl coverUrl;
        bool coverIsSquare = false;
        QString workId;
    };

    explicit ImportQueueModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_entries.size()); }
    int finishedCount() const;
    int attentionCount() const;
    bool active() const;

    // Adds a row at the end and returns its id.
    int append(Entry entry);
    // Replaces the row with this id by the given rows (in its place) and
    // returns their ids; an empty list removes the row.
    QList<int> replace(int id, QList<Entry> entries);
    // Changes the row with this id; does nothing if there is none.
    void update(int id, const std::function<void(Entry &)> &change);
    // The row with this id, nullptr if there is none.
    const Entry *entry(int id) const;

    static bool isRunning(Status status) { return status == Waiting || status == Reading || status == Copying; }

    Q_INVOKABLE void markReviewed(int entryId);
    Q_INVOKABLE void clearFinished();

signals:
    void countChanged();
    void summaryChanged();

private:
    int rowOf(int id) const;

    QList<Entry> m_entries;
    int m_nextId = 1;
};
