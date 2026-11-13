#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QThread>
#include <QUrl>

#include <atomic>

#include "libraryexport.h"

class LibraryController;

// "Exportieren …" for selected works: copies their files into a new folder
// "calibly-Export <date>" in a folder the user chooses, with the subfolders
// "Ebooks" and "Audiobooks" and no further subfolders (see LibraryExport).
// The copying runs in its own thread; ExportHint shows it.
//
// In QML (import Calibly.Controllers):
//   ExportController.exportWorks(workIds, parentFolder)
//       starts the export into a new folder inside parentFolder (a URL from
//       a FolderDialog); false if one is running already or it could not
//       start (state Failed, errorText says why)
//   ExportController.cancel()      stops it and removes the new folder
//   ExportController.dismiss()     hides the result (state Idle)
//   ExportController.openFolder()  shows the export folder in the Explorer
//   state         Idle, Running, Done, Cancelled, Failed
//   running       state == Running
//   fileCount     files to copy
//   copiedCount   files copied so far
//   progress      0 – 1, by bytes
//   currentFile   name of the file being copied
//   folder        the export folder (path), "" before the first export
//   folderName    its name, e.g. "calibly-Export 2026-11-13"
//   missingCount  files of the works that were not found and are left out
//   errorText     why it failed (state Failed)
//   lastFolder    where the last export went, as a URL for the
//                 FolderDialog; the documents folder at first
class ExportController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(State state READ state NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY changed)
    Q_PROPERTY(int copiedCount READ copiedCount NOTIFY progressChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY progressChanged)
    Q_PROPERTY(QString folder READ folder NOTIFY changed)
    Q_PROPERTY(QString folderName READ folderName NOTIFY changed)
    Q_PROPERTY(int missingCount READ missingCount NOTIFY changed)
    Q_PROPERTY(QString errorText READ errorText NOTIFY changed)
    Q_PROPERTY(QUrl lastFolder READ lastFolder NOTIFY changed)

public:
    enum State { Idle, Running, Done, Cancelled, Failed };
    Q_ENUM(State)

    ExportController(LibraryController &library, QQmlEngine *engine, QObject *parent = nullptr);
    ~ExportController() override;

    static ExportController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    Q_INVOKABLE bool exportWorks(const QStringList &workIds, const QUrl &parentFolder);
    bool exportWorksTo(const QStringList &workIds, const QString &parentFolder);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void dismiss();
    Q_INVOKABLE void openFolder();

    State state() const { return m_state; }
    bool running() const { return m_state == Running; }
    int fileCount() const { return int(m_plan.files.size()); }
    int copiedCount() const { return m_copied; }
    double progress() const;
    QString currentFile() const { return m_currentFile; }
    QString folder() const { return m_plan.folder; }
    QString folderName() const;
    int missingCount() const { return int(m_plan.missing.size()); }
    QString errorText() const { return m_errorText; }
    QUrl lastFolder() const;

signals:
    void changed();
    void progressChanged();

private:
    void fail(const QString &text);
    void onProgress(qint64 bytes, int files, const QString &current);
    void onFinished(const LibraryExport::Outcome &outcome);

    static ExportController *s_instance;

    LibraryController &m_library;
    QQmlEngine *m_engine = nullptr;
    QThread *m_thread = nullptr;
    std::atomic_bool m_cancel = false;
    State m_state = Idle;
    LibraryExport::Plan m_plan;
    qint64 m_bytes = 0;
    int m_copied = 0;
    QString m_currentFile;
    QString m_errorText;
};
