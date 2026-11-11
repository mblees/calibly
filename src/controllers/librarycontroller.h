#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QThread>
#include <QUrl>

#include "workmodel.h"

class LibraryBackend;
class ImportWorker;

// Entry point of the views to the library.
//
// In QML (import Calibly.Controllers):
//   LibraryController.works        WorkModel with all works
//   LibraryController.reload()     reads all works again from the backend
//   LibraryController.openFormat(workId, kind)
//       opens the file of a format ("ebook" or "audiobook") with the default
//       application of the system and remembers when; false if the file is
//       missing or no application could open it
//
//   Import (only with a real library, see canImport):
//   LibraryController.importFiles(urls)   files or folders, runs in the background
//   importing, importDone, importTotal, importFileName   progress
//   importFinished(added, skipped, failed)   added = new formats,
//       skipped = already in the library, failed = could not be copied
//   fileNameFilters   patterns for a file dialog, e.g. ["*.epub", "*.m4b", ...]
class LibraryController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(WorkModel *works READ works CONSTANT)
    Q_PROPERTY(bool canImport READ canImport CONSTANT)
    Q_PROPERTY(QString libraryRoot READ libraryRoot CONSTANT)
    Q_PROPERTY(QStringList fileNameFilters READ fileNameFilters CONSTANT)
    Q_PROPERTY(bool importing READ importing NOTIFY importStateChanged)
    Q_PROPERTY(int importDone READ importDone NOTIFY importStateChanged)
    Q_PROPERTY(int importTotal READ importTotal NOTIFY importStateChanged)
    Q_PROPERTY(QString importFileName READ importFileName NOTIFY importStateChanged)

public:
    explicit LibraryController(LibraryBackend &backend, QQmlEngine *engine, QObject *parent = nullptr);
    ~LibraryController() override;

    // Used by the QML engine to obtain the singleton instance created in main().
    static LibraryController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    WorkModel *works() { return &m_works; }

    bool canImport() const;
    QString libraryRoot() const;
    QStringList fileNameFilters() const;
    bool importing() const { return m_importing; }
    int importDone() const { return m_importDone; }
    int importTotal() const { return m_importTotal; }
    QString importFileName() const { return m_importFileName; }

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool openFormat(const QString &workId, const QString &kind);
    Q_INVOKABLE void importFiles(const QList<QUrl> &urls);

signals:
    void importStateChanged();
    void importFinished(int added, int skipped, int failed);

private:
    void onImportProgress(int done, int total, const QString &fileName);
    void onImportFinished(const QList<Work> &works, int added, const QStringList &duplicates,
                          const QStringList &failed);

    LibraryBackend &m_backend;
    QQmlEngine *m_engine = nullptr;
    WorkModel m_works;

    QThread m_importThread;
    ImportWorker *m_importWorker = nullptr;
    bool m_importing = false;
    int m_importDone = 0;
    int m_importTotal = 0;
    QString m_importFileName;

    static inline LibraryController *s_instance = nullptr;
};
