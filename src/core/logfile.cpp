#include "logfile.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QStandardPaths>

namespace {

QMutex mutex;
QString folder;
QFile file;
QDate fileDay;
QtMessageHandler previousHandler = nullptr;

char levelLetter(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return 'D';
    case QtInfoMsg:
        return 'I';
    case QtWarningMsg:
        return 'W';
    case QtCriticalMsg:
        return 'C';
    case QtFatalMsg:
        return 'F';
    }
    return '?';
}

void handler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    {
        const QMutexLocker locker(&mutex);
        const QDateTime now = QDateTime::currentDateTime();
        // A new file when the day changes while calibly runs.
        if (fileDay != now.date()) {
            file.close();
            fileDay = now.date();
            file.setFileName(QDir(folder).filePath(LogFile::fileName(fileDay)));
            if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
                fileDay = QDate();
        }
        if (file.isOpen()) {
            const QString category = context.category && qstrcmp(context.category, "default") != 0
                                         ? QString::fromLatin1(context.category) + QStringLiteral(": ")
                                         : QString();
            const QString line = now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")) + u' '
                                 + QLatin1Char(levelLetter(type)) + u' ' + category + message + u'\n';
            file.write(line.toUtf8());
            file.flush();
        }
    }
    if (previousHandler)
        previousHandler(type, context, message);
}

} // namespace

namespace LogFile {

QString defaultDirectory()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
        .filePath(QStringLiteral("calibly/logs"));
}

QString directory()
{
    const QMutexLocker locker(&mutex);
    return folder;
}

QString fileName(const QDate &day)
{
    return QStringLiteral("calibly-%1.log").arg(day.toString(Qt::ISODate));
}

void removeOld(const QString &directory, const QDate &today, int keepDays)
{
    const QDir dir(directory);
    const QDate oldest = today.addDays(-keepDays);
    for (const QString &name : dir.entryList({ QStringLiteral("calibly-*.log") }, QDir::Files)) {
        const QDate day = QDate::fromString(name.mid(8, 10), Qt::ISODate);
        if (day.isValid() && day < oldest)
            QFile::remove(dir.filePath(name));
    }
}

void install(const QString &directory)
{
    const QString target = directory.isEmpty() ? defaultDirectory() : directory;
    if (!QDir().mkpath(target))
        return; // no log file, the console still works
    removeOld(target, QDate::currentDate());
    {
        const QMutexLocker locker(&mutex);
        folder = target;
    }
    previousHandler = qInstallMessageHandler(handler);
}

} // namespace LogFile
