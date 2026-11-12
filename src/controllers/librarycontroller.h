#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QUrl>

#include "workmodel.h"

class LibraryBackend;

// Entry point of the views to the library.
//
// In QML (import Calibly.Controllers):
//   LibraryController.works        WorkModel with all works
//   LibraryController.reload()     reads all works again from the backend
//   LibraryController.openFormat(workId, kind)
//       opens the file of a format ("ebook" or "audiobook") with the default
//       application of the system and remembers when; false if the file is
//       missing or no application could open it
//   LibraryController.setFinished(workId, kind, finished)
//       marks a format as read / listened to the end, or takes that back
//       (an audiobook then is where it was stopped, an ebook at the start)
//
//   canImport         files can be imported (a real library, not --mock);
//                     importing itself is done by ImportController
//   fileNameFilters   patterns for a file dialog, e.g. ["*.epub", "*.m4b", ...]
//
//   LibraryController.mergeWorks(workIds)
//       "Zu einem Werk verbinden": joins the works into one (see
//       mergeWorks() in core) and removes the others; their files stay where
//       they are. Returns the id of the work that is left, "" on failure.
//       One id joins the audiobook files of that work into one audiobook.
//   LibraryController.mergeTitle(workIds)   title of the work that would be left
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
    Q_PROPERTY(bool canImport READ canImport CONSTANT)
    Q_PROPERTY(QString libraryRoot READ libraryRoot CONSTANT)
    Q_PROPERTY(QStringList fileNameFilters READ fileNameFilters CONSTANT)

public:
    explicit LibraryController(LibraryBackend &backend, QQmlEngine *engine, QObject *parent = nullptr);
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

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool openFormat(const QString &workId, const QString &kind);
    Q_INVOKABLE bool setFinished(const QString &workId, const QString &kind, bool finished);
    Q_INVOKABLE QString mergeWorks(const QStringList &workIds);
    Q_INVOKABLE QString mergeTitle(const QStringList &workIds) const;
    Q_INVOKABLE int removeWorks(const QStringList &workIds);

    // Removed files are deleted instead of moved to the recycle bin (tests).
    void setDeleteFilesPermanently(bool permanently) { m_trashFiles = !permanently; }

    // A file of this format was (not) found, e.g. when it was opened. Only
    // changes what the views show; it is checked again when the library is
    // loaded. Ignored for sample data, whose files never exist.
    void setFileMissing(const QString &workId, int formatIndex, bool missing);

signals:
    void worksAboutToChange(const QStringList &workIds);

private:
    // The works with these ids, in this order; unknown ids are left out.
    QList<Work> worksById(const QStringList &workIds) const;

    LibraryBackend &m_backend;
    bool m_trashFiles = true;
    QQmlEngine *m_engine = nullptr;
    WorkModel m_works;

    static inline LibraryController *s_instance = nullptr;
};
