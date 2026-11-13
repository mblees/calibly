// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "workdetails.h"

#include <QFileInfo>
#include <QVariantMap>

#include "audiotimeline.h"
#include "textutils.h"
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
WORK_FIELD(QString, languageName, TextUtils::languageName(m_work->language), QString())
WORK_FIELD(int, publishedYear, m_work->publishedYear, 0)
WORK_FIELD(QDateTime, added, m_work->added, QDateTime())
WORK_FIELD(bool, coverIsSquare, m_work->coverIsSquare(m_work->primaryKind()), false)
WORK_FIELD(bool, audiobookCoverIsSquare, m_work->coverIsSquare(Format::Kind::Audiobook), true)

#undef WORK_FIELD

namespace {

QString formatField(const Work *work, Format::Kind kind, QString Format::*field)
{
    const Format *format = work ? work->format(kind) : nullptr;
    return format ? format->*field : QString();
}

} // namespace

QString WorkDetails::ebookPublisher() const
{
    return formatField(m_work, Format::Kind::Ebook, &Format::publisher);
}

QString WorkDetails::audiobookPublisher() const
{
    return formatField(m_work, Format::Kind::Audiobook, &Format::publisher);
}

QString WorkDetails::isbn() const
{
    const QString ebook = formatField(m_work, Format::Kind::Ebook, &Format::isbn);
    return ebook.isEmpty() ? formatField(m_work, Format::Kind::Audiobook, &Format::isbn) : ebook;
}

QUrl WorkDetails::audiobookCoverUrl() const
{
    if (!m_model)
        return {};
    const int row = m_model->rowOf(m_workId);
    return row < 0 ? QUrl() : m_model->index(row).data(WorkModel::AudiobookCoverUrlRole).toUrl();
}

QVariantList WorkDetails::chapters() const
{
    QVariantList list;
    const Format *audiobook = m_work ? m_work->format(Format::Kind::Audiobook) : nullptr;
    if (!audiobook)
        return list;
    for (const AudioTimeline::Entry &chapter : AudioTimeline(*audiobook).chapters()) {
        list.append(QVariantMap{ { QStringLiteral("title"), chapter.title },
                                 { QStringLiteral("start"), double(chapter.startMs) },
                                 { QStringLiteral("duration"), double(chapter.durationMs) } });
    }
    return list;
}

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
    const bool volumes = m_work->isCombined() && m_work->formatCount(Format::Kind::Ebook) > 1;
    int volume = 0;
    for (int index = 0; index < m_work->formats.size(); ++index) {
        const Format &format = m_work->formats.at(index);
        const QFileInfo file(format.filePath);
        const bool isVolume = volumes && format.kind == Format::Kind::Ebook;
        list.append(QVariantMap{
            { QStringLiteral("index"), index },
            { QStringLiteral("volumeNumber"), isVolume ? ++volume : 0 },
            { QStringLiteral("volumeTitle"), isVolume ? m_work->formatTitle(index) : QString() },
            { QStringLiteral("kind"), format.kind == Format::Kind::Audiobook ? QStringLiteral("audiobook")
                                                                             : QStringLiteral("ebook") },
            { QStringLiteral("fileName"), file.fileName() },
            { QStringLiteral("fileType"), file.suffix().toUpper() },
            { QStringLiteral("fileSize"), double(format.fileSize) },
            { QStringLiteral("pages"), format.pages },
            { QStringLiteral("durationSeconds"), format.durationSeconds },
            { QStringLiteral("progress"), format.progress },
            { QStringLiteral("publisher"), format.publisher },
            { QStringLiteral("isbn"), format.isbn },
            { QStringLiteral("available"), !format.filePath.isEmpty() && file.exists() },
        });
    }
    return list;
}
