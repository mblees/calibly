// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "exportcontroller.h"

#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QLocale>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>

#include "librarycontroller.h"

namespace {

const QString kLastFolderKey = QStringLiteral("export/lastFolder");

// Progress reaches the UI at most this often (milliseconds).
constexpr qint64 kProgressInterval = 50;

} // namespace

ExportController *ExportController::s_instance = nullptr;

ExportController::ExportController(LibraryController &library, QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_library(library)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

ExportController::~ExportController()
{
    if (m_thread) {
        m_cancel = true;
        m_thread->wait();
        delete m_thread;
    }
    s_instance = nullptr;
}

ExportController *ExportController::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

bool ExportController::exportWorks(const QStringList &workIds, const QUrl &parentFolder)
{
    return exportWorksTo(workIds, parentFolder.toLocalFile());
}

bool ExportController::exportWorksTo(const QStringList &workIds, const QString &parentFolder)
{
    if (running())
        return false;
    m_plan = {};
    m_bytes = 0;
    m_copied = 0;
    m_currentFile.clear();
    m_errorText.clear();

    if (parentFolder.isEmpty() || !QFileInfo(parentFolder).isDir()) {
        fail(tr("Der gewählte Ordner wurde nicht gefunden."));
        return false;
    }
    const QString parent = QDir::cleanPath(QFileInfo(parentFolder).absoluteFilePath());
    const QString folder = LibraryExport::uniqueFolder(parent, LibraryExport::folderName(QDate::currentDate()));
    m_plan = LibraryExport::plan(m_library.worksById(workIds), folder);
    if (m_plan.files.isEmpty()) {
        fail(m_plan.missing.isEmpty() ? tr("Die ausgewählten Titel haben keine Dateien.")
                                      : tr("Die Dateien der ausgewählten Titel wurden nicht gefunden."));
        return false;
    }
    const QStorageInfo storage(parent);
    if (storage.isValid() && storage.bytesAvailable() >= 0 && storage.bytesAvailable() < m_plan.totalBytes) {
        const QLocale locale;
        fail(tr("Nicht genug Platz: Der Export braucht %1, frei sind %2.")
                 .arg(locale.formattedDataSize(m_plan.totalBytes), locale.formattedDataSize(storage.bytesAvailable())));
        return false;
    }
    QSettings().setValue(kLastFolderKey, parent);

    m_state = Running;
    m_cancel = false;
    emit changed();
    emit progressChanged();

    const LibraryExport::Plan plan = m_plan;
    m_thread = QThread::create([this, plan] {
        QElapsedTimer timer;
        timer.start();
        int lastFiles = 0;
        const auto progress = [this, &timer, &lastFiles](qint64 bytes, int files, const QString &current) {
            if (files == lastFiles && timer.elapsed() < kProgressInterval)
                return;
            lastFiles = files;
            timer.restart();
            QMetaObject::invokeMethod(this, [this, bytes, files, current] { onProgress(bytes, files, current); },
                                      Qt::QueuedConnection);
        };
        const LibraryExport::Outcome outcome = LibraryExport::run(plan, progress, m_cancel);
        QMetaObject::invokeMethod(this, [this, outcome] { onFinished(outcome); }, Qt::QueuedConnection);
    });
    m_thread->start();
    return true;
}

void ExportController::cancel()
{
    if (running())
        m_cancel = true;
}

void ExportController::dismiss()
{
    if (running() || m_state == Idle)
        return;
    m_state = Idle;
    emit changed();
}

void ExportController::openFolder()
{
    if (!m_plan.folder.isEmpty() && QFileInfo(m_plan.folder).isDir())
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_plan.folder));
}

double ExportController::progress() const
{
    if (m_plan.totalBytes > 0)
        return std::min(1.0, double(m_bytes) / double(m_plan.totalBytes));
    return m_plan.files.isEmpty() ? 0.0 : double(m_copied) / double(m_plan.files.size());
}

QString ExportController::folderName() const
{
    return m_plan.folder.isEmpty() ? QString() : QFileInfo(m_plan.folder).fileName();
}

QUrl ExportController::lastFolder() const
{
    QString folder = QSettings().value(kLastFolderKey).toString();
    if (folder.isEmpty() || !QFileInfo(folder).isDir())
        folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QUrl::fromLocalFile(folder);
}

void ExportController::fail(const QString &text)
{
    m_errorText = text;
    m_state = Failed;
    emit changed();
    emit progressChanged();
}

void ExportController::onProgress(qint64 bytes, int files, const QString &current)
{
    if (!running())
        return;
    m_bytes = bytes;
    m_copied = files;
    m_currentFile = current;
    emit progressChanged();
}

void ExportController::onFinished(const LibraryExport::Outcome &outcome)
{
    if (m_thread) {
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;
    }
    m_copied = outcome.copied;
    m_currentFile.clear();
    switch (outcome.result) {
    case LibraryExport::Result::Done:
        m_bytes = m_plan.totalBytes;
        m_state = Done;
        break;
    case LibraryExport::Result::Cancelled:
        m_state = Cancelled;
        break;
    case LibraryExport::Result::CannotCreateFolder:
        m_errorText = tr("Der Export-Ordner kann dort nicht angelegt werden.");
        m_state = Failed;
        break;
    case LibraryExport::Result::CannotCopy:
        m_errorText = tr("„%1“ konnte nicht kopiert werden. %2 von %3 Dateien liegen im Export-Ordner.")
                          .arg(QFileInfo(outcome.failedFile).fileName())
                          .arg(outcome.copied)
                          .arg(m_plan.files.size());
        m_state = Failed;
        break;
    }
    emit changed();
    emit progressChanged();
}
