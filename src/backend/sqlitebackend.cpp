#include "sqlitebackend.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

#include <utility>

namespace {

// 1: phase 4
// 2: publisher per format instead of per work, ISBN per format (phase 5)
// 3: chapters and listening position per format (phase 6)
// 4: last opened per format, for "Continue" (phase 8)
// 5: reading position per format, for the ebook reader (phase 10)
constexpr int kSchemaVersion = 5;

QString toJson(const QStringList &list)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(list)).toJson(QJsonDocument::Compact));
}

QStringList fromJson(const QString &json)
{
    QStringList list;
    const QJsonArray array = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const QJsonValue &value : array)
        list.append(value.toString());
    return list;
}

QString toText(const QDateTime &dateTime)
{
    return dateTime.isValid() ? dateTime.toString(Qt::ISODate) : QString();
}

bool exec(QSqlQuery &query, QString *error)
{
    if (query.exec())
        return true;
    if (error)
        *error = query.lastError().text();
    qWarning("SqliteBackend: %s", qPrintable(query.lastError().text()));
    return false;
}

// The file of the format, or one of the files of an audiobook of several
// files, does not exist.
bool fileMissing(const Format &format)
{
    if (format.filePath.isEmpty() || !QFileInfo::exists(format.filePath))
        return true;
    QString checked;
    for (const Chapter &chapter : format.chapters) {
        if (chapter.filePath.isEmpty() || chapter.filePath == checked)
            continue; // chapters of one file follow each other
        checked = chapter.filePath;
        if (!QFileInfo::exists(checked))
            return true;
    }
    return false;
}

} // namespace

SqliteBackend::SqliteBackend(QString libraryRoot)
    : m_root(std::move(libraryRoot))
    , m_connectionName(QStringLiteral("calibly-") + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

SqliteBackend::~SqliteBackend()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::database(m_connectionName).close();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

QString SqliteBackend::defaultLibraryRoot()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath(QStringLiteral("calibly-Bibliothek"));
}

bool SqliteBackend::open(QString *error)
{
    if (!QDir().mkpath(m_root)) {
        if (error)
            *error = QStringLiteral("cannot create library folder %1").arg(m_root);
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(QDir(m_root).filePath(QStringLiteral("calibly.db")));
    if (!db.open()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    query.exec(QStringLiteral("PRAGMA user_version"));
    const int version = query.next() ? query.value(0).toInt() : 0;
    if (version > kSchemaVersion) {
        if (error)
            *error = QStringLiteral("library was created by a newer calibly version");
        return false;
    }
    // Each step brings the database from one version to the next; a new
    // database runs through all of them.
    QStringList statements;
    if (version < 1) {
        statements += {
            QStringLiteral("CREATE TABLE IF NOT EXISTS works ("
                           " id TEXT PRIMARY KEY, title TEXT NOT NULL, authors TEXT, narrators TEXT,"
                           " series TEXT, series_index INTEGER, description TEXT, tags TEXT,"
                           " rating INTEGER, language TEXT, published_year INTEGER, publisher TEXT,"
                           " added TEXT, last_opened TEXT)"), // publisher: only up to version 1
            QStringLiteral("CREATE TABLE IF NOT EXISTS formats ("
                           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           " work_id TEXT NOT NULL REFERENCES works(id) ON DELETE CASCADE,"
                           " position INTEGER, kind TEXT NOT NULL, file_path TEXT, progress REAL,"
                           " cover_path TEXT, cover_square INTEGER, file_size INTEGER, pages INTEGER,"
                           " duration INTEGER)"),
            QStringLiteral("CREATE INDEX IF NOT EXISTS formats_work ON formats(work_id)"),
        };
    }
    if (version < 2) {
        statements += {
            QStringLiteral("ALTER TABLE formats ADD COLUMN publisher TEXT"),
            QStringLiteral("ALTER TABLE formats ADD COLUMN isbn TEXT"),
            QStringLiteral("UPDATE formats SET publisher = (SELECT works.publisher FROM works"
                           " WHERE works.id = formats.work_id)"),
            QStringLiteral("ALTER TABLE works DROP COLUMN publisher"),
        };
    }
    if (version < 3) {
        statements += {
            QStringLiteral("ALTER TABLE formats ADD COLUMN chapters TEXT"), // JSON, see chaptersToJson()
            QStringLiteral("ALTER TABLE formats ADD COLUMN position_ms INTEGER"),
        };
    }
    if (version < 4) {
        // Older works get it from the work's last_opened, see Work::inheritLastOpened().
        statements += { QStringLiteral("ALTER TABLE formats ADD COLUMN last_opened TEXT") };
    }
    if (version < 5) {
        // JSON, see ReadingPosition.
        statements += { QStringLiteral("ALTER TABLE formats ADD COLUMN reading_position TEXT") };
    }
    if (statements.isEmpty())
        return true;

    statements.append(QStringLiteral("PRAGMA user_version = %1").arg(kSchemaVersion));
    db.transaction();
    for (const QString &statement : std::as_const(statements)) {
        query.prepare(statement);
        if (!exec(query, error)) {
            db.rollback();
            return false;
        }
    }
    return db.commit();
}

// [{ "title": …, "file": relative path or absent, "start": ms, "duration": ms }]
QString SqliteBackend::chaptersToJson(const QList<Chapter> &chapters) const
{
    if (chapters.isEmpty())
        return QString();
    QJsonArray array;
    for (const Chapter &chapter : chapters) {
        QJsonObject object{ { QStringLiteral("title"), chapter.title },
                            { QStringLiteral("start"), chapter.startMs },
                            { QStringLiteral("duration"), chapter.durationMs } };
        if (!chapter.filePath.isEmpty())
            object.insert(QStringLiteral("file"), relative(chapter.filePath));
        array.append(object);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QList<Chapter> SqliteBackend::chaptersFromJson(const QString &json) const
{
    QList<Chapter> chapters;
    const QJsonArray array = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        chapters.append({ object.value(QStringLiteral("title")).toString(),
                          absolute(object.value(QStringLiteral("file")).toString()),
                          object.value(QStringLiteral("start")).toInteger(),
                          object.value(QStringLiteral("duration")).toInteger() });
    }
    return chapters;
}

QString SqliteBackend::absolute(const QString &relativePath) const
{
    return relativePath.isEmpty() ? QString() : QDir::cleanPath(QDir(m_root).filePath(relativePath));
}

QString SqliteBackend::relative(const QString &absolutePath) const
{
    return absolutePath.isEmpty() ? QString() : QDir(m_root).relativeFilePath(absolutePath);
}

QList<Work> SqliteBackend::loadWorks()
{
    QList<Work> works;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen())
        return works;

    QHash<QString, int> indexById;
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id, title, authors, narrators, series, series_index, description, tags,"
                                 " rating, language, published_year, added, last_opened FROM works"));
    if (!exec(query, nullptr))
        return works;
    while (query.next()) {
        Work work;
        work.id = query.value(0).toString();
        work.title = query.value(1).toString();
        work.authors = fromJson(query.value(2).toString());
        work.narrators = fromJson(query.value(3).toString());
        work.series = query.value(4).toString();
        work.seriesIndex = query.value(5).toInt();
        work.description = query.value(6).toString();
        work.tags = fromJson(query.value(7).toString());
        work.rating = query.value(8).toInt();
        work.language = query.value(9).toString();
        work.publishedYear = query.value(10).toInt();
        work.added = QDateTime::fromString(query.value(11).toString(), Qt::ISODate);
        work.lastOpened = QDateTime::fromString(query.value(12).toString(), Qt::ISODate);
        indexById.insert(work.id, int(works.size()));
        works.append(work);
    }

    query.prepare(QStringLiteral("SELECT work_id, kind, file_path, progress, cover_path, cover_square, file_size,"
                                 " pages, duration, publisher, isbn, chapters, position_ms, last_opened,"
                                 " reading_position FROM formats"
                                 " ORDER BY work_id, position"));
    if (!exec(query, nullptr))
        return works;
    while (query.next()) {
        const int index = indexById.value(query.value(0).toString(), -1);
        if (index < 0)
            continue;
        Format format;
        format.kind = query.value(1).toString() == QLatin1String("audiobook") ? Format::Kind::Audiobook
                                                                             : Format::Kind::Ebook;
        format.filePath = absolute(query.value(2).toString());
        format.progress = query.value(3).toDouble();
        format.coverPath = absolute(query.value(4).toString());
        format.coverIsSquare = query.value(5).toBool();
        format.fileSize = query.value(6).toLongLong();
        format.pages = query.value(7).toInt();
        format.durationSeconds = query.value(8).toInt();
        format.publisher = query.value(9).toString();
        format.isbn = query.value(10).toString();
        format.chapters = chaptersFromJson(query.value(11).toString());
        format.positionMs = query.value(12).toLongLong();
        format.lastOpened = QDateTime::fromString(query.value(13).toString(), Qt::ISODate);
        format.readingPosition = query.value(14).toString();
        format.fileMissing = fileMissing(format);
        works[index].formats.append(format);
    }
    for (Work &work : works)
        work.inheritLastOpened();
    return works;
}

bool SqliteBackend::removeWork(const QString &workId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen() || workId.isEmpty())
        return false;

    db.transaction();
    QSqlQuery query(db);
    // Explicitly, not only by ON DELETE CASCADE: that needs the foreign key
    // pragma, which is set per connection.
    query.prepare(QStringLiteral("DELETE FROM formats WHERE work_id = ?"));
    query.addBindValue(workId);
    bool ok = exec(query, nullptr);
    if (ok) {
        query.prepare(QStringLiteral("DELETE FROM works WHERE id = ?"));
        query.addBindValue(workId);
        ok = exec(query, nullptr) && query.numRowsAffected() > 0;
    }
    if (ok)
        return db.commit();
    db.rollback();
    return false;
}

bool SqliteBackend::saveWork(const Work &work)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen() || work.id.isEmpty())
        return false;

    db.transaction();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO works (id, title, authors, narrators, series, series_index,"
                                 " description, tags, rating, language, published_year, added, last_opened)"
                                 " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
                                 " ON CONFLICT(id) DO UPDATE SET title = excluded.title, authors = excluded.authors,"
                                 " narrators = excluded.narrators, series = excluded.series,"
                                 " series_index = excluded.series_index, description = excluded.description,"
                                 " tags = excluded.tags, rating = excluded.rating, language = excluded.language,"
                                 " published_year = excluded.published_year,"
                                 " added = excluded.added, last_opened = excluded.last_opened"));
    const QVariantList values = { work.id, work.title, toJson(work.authors), toJson(work.narrators), work.series,
                                  work.seriesIndex, work.description, toJson(work.tags), work.rating, work.language,
                                  work.publishedYear, toText(work.added), toText(work.lastOpened) };
    for (const QVariant &value : values)
        query.addBindValue(value);
    bool ok = exec(query, nullptr);

    if (ok) {
        query.prepare(QStringLiteral("DELETE FROM formats WHERE work_id = ?"));
        query.addBindValue(work.id);
        ok = exec(query, nullptr);
    }
    for (int position = 0; ok && position < work.formats.size(); ++position) {
        const Format &format = work.formats.at(position);
        query.prepare(QStringLiteral("INSERT INTO formats (work_id, position, kind, file_path, progress, cover_path,"
                                     " cover_square, file_size, pages, duration, publisher, isbn, chapters, position_ms,"
                                     " last_opened, reading_position)"
                                     " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        const QVariantList formatValues = {
            work.id, position,
            format.kind == Format::Kind::Audiobook ? QStringLiteral("audiobook") : QStringLiteral("ebook"),
            relative(format.filePath), format.progress, relative(format.coverPath), format.coverIsSquare,
            format.fileSize, format.pages, format.durationSeconds, format.publisher, format.isbn,
            chaptersToJson(format.chapters), format.positionMs, toText(format.lastOpened), format.readingPosition };
        for (const QVariant &value : formatValues)
            query.addBindValue(value);
        ok = exec(query, nullptr);
    }

    if (ok)
        return db.commit();
    db.rollback();
    return false;
}
