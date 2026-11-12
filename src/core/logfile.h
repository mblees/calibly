#pragma once

#include <QDate>
#include <QString>

// The log of calibly (release step 3): every Qt message (qDebug, qInfo,
// qWarning, ...) is also written to a file, one per day
// (calibly-2026-11-12.log), so a user can send it with a bug report. Files
// older than 14 days are deleted at start. The messages still go to the
// console as before.
namespace LogFile {

// Default folder: %LOCALAPPDATA%\calibly\logs
QString defaultDirectory();
// The folder in use, "" before install().
QString directory();

// Starts writing to the folder (default: defaultDirectory()). Call once,
// after the QCoreApplication exists.
void install(const QString &directory = QString());

// Deletes the log files of days before today - keepDays.
void removeOld(const QString &directory, const QDate &today, int keepDays = 14);

// "calibly-2026-11-12.log"
QString fileName(const QDate &day);

} // namespace LogFile
