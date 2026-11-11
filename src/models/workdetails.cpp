#include "workdetails.h"

#include <QFileInfo>
#include <QVariantMap>

#include "workmodel.h"

WorkDetails::WorkDetails(QObject *parent)
    : QObject(parent)
{
}

void WorkDetails::setModel(WorkModel *model)
{
    if (model == m_model)
        return;
    for (const QMetaObject::Connection &connection : std::as_const(m_connections))
        disconnect(connection);
    m_connections.clear();

    m_model = model;
    if (m_model) {
        // Any change of the model may move or change the work: look it up again.
        const auto update = [this] { refresh(); };
        m_connections = {
            // The stored work becomes invalid while the model is reset.
            connect(m_model, &QAbstractItemModel::modelAboutToBeReset, this, [this] { m_work = nullptr; }),
            connect(m_model, &QAbstractItemModel::modelReset, this, update),
            connect(m_model, &QAbstractItemModel::rowsInserted, this, update),
            connect(m_model, &QAbstractItemModel::rowsRemoved, this, update),
            connect(m_model, &QAbstractItemModel::dataChanged, this, update),
        };
    }
    emit modelChanged();
    refresh();
}

void WorkDetails::setWorkId(const QString &workId)
{
    if (workId == m_workId)
        return;
    m_workId = workId;
    emit workIdChanged();
    refresh();
}

void WorkDetails::refresh()
{
    m_work = m_model ? m_model->workAt(m_model->rowOf(m_workId)) : nullptr;
    emit changed();
}

#define WORK_FIELD(type, name, expression, fallback) \
    type WorkDetails::name() const { return m_work ? type(expression) : type(fallback); }

WORK_FIELD(QString, title, m_work->title, QString())
WORK_FIELD(QStringList, authors, m_work->authors, QStringList())
WORK_FIELD(QStringList, narrators, m_work->narrators, QStringList())
WORK_FIELD(QString, series, m_work->series, QString())
WORK_FIELD(int, seriesIndex, m_work->seriesIndex, 0)
WORK_FIELD(QString, description, m_work->description, QString())
WORK_FIELD(QStringList, tags, m_work->tags, QStringList())
WORK_FIELD(int, rating, m_work->rating, 0)
WORK_FIELD(QString, language, m_work->language, QString())
WORK_FIELD(int, publishedYear, m_work->publishedYear, 0)
WORK_FIELD(QString, publisher, m_work->publisher, QString())
WORK_FIELD(QDateTime, added, m_work->added, QDateTime())
WORK_FIELD(bool, coverIsSquare, m_work->coverIsSquare(m_work->primaryKind()), false)

#undef WORK_FIELD

QUrl WorkDetails::coverUrl() const
{
    if (!m_model)
        return {};
    const int row = m_model->rowOf(m_workId);
    return row < 0 ? QUrl() : m_model->index(row).data(WorkModel::CoverUrlRole).toUrl();
}

QVariantList WorkDetails::formats() const
{
    QVariantList list;
    if (!m_work)
        return list;
    for (const Format &format : m_work->formats) {
        const QFileInfo file(format.filePath);
        list.append(QVariantMap{
            { QStringLiteral("kind"), format.kind == Format::Kind::Audiobook ? QStringLiteral("audiobook")
                                                                             : QStringLiteral("ebook") },
            { QStringLiteral("fileName"), file.fileName() },
            { QStringLiteral("fileType"), file.suffix().toUpper() },
            { QStringLiteral("fileSize"), double(format.fileSize) },
            { QStringLiteral("pages"), format.pages },
            { QStringLiteral("durationSeconds"), format.durationSeconds },
            { QStringLiteral("progress"), format.progress },
            { QStringLiteral("available"), !format.filePath.isEmpty() && file.exists() },
        });
    }
    return list;
}
