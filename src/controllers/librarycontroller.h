// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QUrl>

#include <memory>

#include "workmodel.h"

class LibraryBackend;
class LibraryLock;

// Entry point of the views to the library.
//
// In QML (import Calibly.Controllers):
//   LibraryController.works        WorkModel with all works
//   LibraryController.reload()     reads all works again from the backend
//   LibraryController.openFormat(workId, kind)
//       opens the file of a format ("ebook" or "audiobook") with the default
//       application of the system and remembers when; false if the file is
//       missing or no application could open it. Of several formats of that
//       kind (the volumes of a combined work) the current one, see
//       Work::currentFormat().
//   LibraryController.openFormatAt(workId, formatIndex)   that format
//   LibraryController.setFinished(workId, kind, finished)
//       marks all formats of that kind as read / listened to the end, or
//       takes that back (an audiobook then is where it was stopped, an ebook
//       at the start)
//   LibraryController.setFormatFinished(workId, formatIndex, finished)
//       the same for one format, e.g. one volume
//   LibraryController.markWorks(workIds, finished)
//       finished: every format of the works read / listened to the end;
//       otherwise "new": progress, positions and "last opened" cleared.
//       Returns how many works were changed.
//   LibraryController.setSeries(workIds, name)
//       "Als Serie zusammenfassen": the works get this series; a volume
//       number they have stays, the others get the next free numbers in the
//       given order. Returns how many works were changed.
//   LibraryController.removeSeries(name)
//       "Serie auflösen": every work of that series (compared as the stacks
//       do, without case and accents) loses its series and volume number.
//   LibraryController.clearSeries(workIds)
//       "Aus der Serie nehmen": the same for these works.
//       Both return how many works were changed.
//   seriesNames       the series of the library, sorted, for suggestions
//   LibraryController.suggestedSeries(workIds)   the first series one of
//       the works has, "" if none has one
//
//   canImport         files can be imported (a real library, not --mock);
//                     importing itself is done by ImportController
//   libraryRoot       the folder of the open library, "" = none / sample data
//
// The library (release step 3): main() opens the last used one with
// openLibrary(); the user can open another one or create a new one.
//   libraryError      why no library is open: NoError (one is open),
//                     NotFound, CannotCreate, Damaged, NewerVersion,
//                     CannotUpdate, InUse (see SqliteBackend::OpenError)
//   libraryPath       the folder of the open library, or of the one that
//                     could not be opened
//   LibraryController.openLibrary(folder, create)
//   LibraryController.openLibraryUrl(url, create)   for a FolderDialog
//       opens the library in the folder (create: a new one if there is
//       none). Returns the error; on success the works of the new library
//       replace the old ones. If one was open and the other cannot be opened,
//       the open one stays.
//   LibraryController.retryLibrary()   tries libraryPath again
//   activationRequested()   calibly was started a second time with this
//       library: the window should come to the front
//   fileNameFilters   patterns for a file dialog, e.g. ["*.epub", "*.m4b", ...]
//
//   LibraryController.mergeWorks(workIds)
//       "Kombinieren": joins the works into one (see mergeWorks() in core)
//       and removes the others; their files stay where they are and the
//       combined work remembers them. Returns the id of the work that is
//       left, "" on failure. One id joins the audiobook files of that work
//       into one audiobook.
//   LibraryController.mergeTitle(workIds)   title of the work that would be left
//   LibraryController.splitWork(workId)
//       "Kombination auflösen": the works it was combined from come back
//       (see splitWork() in core); works merged before phase 12 become one
//       work per format, titled from the file's metadata. Returns how many
//       works there are afterwards, 0 on failure.
//   LibraryController.removeWorks(workIds)
//       removes the works from the library and moves their files in the
//       library folder to the recycle bin; returns how many were removed
//   worksAboutToChange(workIds)   emitted before works are merged or removed,
//       so whoever uses one of them (the player) can let go of it first
//
//   Editing metadata: see WorkEditor, which saves through saveWork().
class LibraryController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(WorkModel *works READ works CONSTANT)
    Q_PROPERTY(bool canImport READ canImport NOTIFY libraryChanged)
    Q_PROPERTY(QString libraryRoot READ libraryRoot NOTIFY libraryChanged)
    Q_PROPERTY(LibraryError libraryError READ libraryError NOTIFY libraryChanged)
    Q_PROPERTY(QString libraryPath READ libraryPath NOTIFY libraryChanged)
    Q_PROPERTY(QStringList fileNameFilters READ fileNameFilters CONSTANT)
    Q_PROPERTY(QStringList seriesNames READ seriesNames NOTIFY seriesNamesChanged)

public:
    enum LibraryError { NoError, NotFound, CannotCreate, Damaged, NewerVersion, CannotUpdate, InUse };
    Q_ENUM(LibraryError)

    // With a backend that stays fixed (sample data, tests).
    explicit LibraryController(LibraryBackend &backend, QQmlEngine *engine, QObject *parent = nullptr);
    // Without a library until openLibrary().
    explicit LibraryController(QQmlEngine *engine, QObject *parent = nullptr);
    ~LibraryController() override;

    // Used by the QML engine to obtain the singleton instance created in main().
    static LibraryController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // The instance created in main(), nullptr before / after.
    static LibraryController *instance() { return s_instance; }

    WorkModel *works() { return &m_works; }
    const WorkModel *works() const { return &m_works; }

    // Stores the work in the backend and updates the model, so every view
    // shows the change immediately; a new work is added. False if the
    // backend cannot store it.
    bool saveWork(const Work &work);

    bool canImport() const;
    QString libraryRoot() const;
    QStringList fileNameFilters() const;
    LibraryError libraryError() const { return m_libraryError; }
    QString libraryPath() const { return m_libraryPath; }

    Q_INVOKABLE LibraryController::LibraryError openLibrary(const QString &folder, bool create);
    Q_INVOKABLE LibraryController::LibraryError openLibraryUrl(const QUrl &folder, bool create);
    Q_INVOKABLE LibraryController::LibraryError retryLibrary();
    // Store the folder of every opened library as the one to open next time
    // (off with --library).
    void setRememberLibrary(bool remember) { m_rememberLibrary = remember; }

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool openFormat(const QString &workId, const QString &kind);
    Q_INVOKABLE bool openFormatAt(const QString &workId, int formatIndex);
    Q_INVOKABLE bool setFinished(const QString &workId, const QString &kind, bool finished);
    Q_INVOKABLE bool setFormatFinished(const QString &workId, int formatIndex, bool finished);
    Q_INVOKABLE int markWorks(const QStringList &workIds, bool finished);
    Q_INVOKABLE int setSeries(const QStringList &workIds, const QString &name);
    Q_INVOKABLE int removeSeries(const QString &name);
    Q_INVOKABLE int clearSeries(const QStringList &workIds);
    Q_INVOKABLE QString mergeWorks(const QStringList &workIds);
    Q_INVOKABLE QString mergeTitle(const QStringList &workIds) const;
    Q_INVOKABLE int splitWork(const QString &workId);
    Q_INVOKABLE int removeWorks(const QStringList &workIds);
    QStringList seriesNames() const;
    Q_INVOKABLE QString suggestedSeries(const QStringList &workIds) const;

    // Removed files are deleted instead of moved to the recycle bin (tests).
    void setDeleteFilesPermanently(bool permanently) { m_trashFiles = !permanently; }

    // A file of this format was (not) found, e.g. when it was opened. Only
    // changes what the views show; it is checked again when the library is
    // loaded. Ignored for sample data, whose files never exist.
    void setFileMissing(const QString &workId, int formatIndex, bool missing);

    // The works with these ids, in this order; unknown ids are left out.
    QList<Work> worksById(const QStringList &workIds) const;

signals:
    void worksAboutToChange(const QStringList &workIds);
    void libraryChanged();
    void seriesNamesChanged();
    void activationRequested();

private:
    void connectModel();

    LibraryBackend *m_backend = nullptr;
    std::unique_ptr<LibraryBackend> m_ownedBackend;
    std::unique_ptr<LibraryLock> m_lock;
    LibraryError m_libraryError = NoError;
    QString m_libraryPath;
    bool m_rememberLibrary = false;
    bool m_trashFiles = true;
    QQmlEngine *m_engine = nullptr;
    WorkModel m_works;

    static inline LibraryController *s_instance = nullptr;
};
