// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "workeditor.h"

#include <QClipboard>
#include <QCollator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QMimeData>
#include <QVariantMap>

#include <algorithm>

#include "librarycontroller.h"
#include "libraryfolder.h"
#include "metadatareader.h"
#include "textutils.h"

namespace {

// Languages offered in the editor, in addition to the work's own one.
const QStringList kCommonLanguages = {
    QStringLiteral("de"), QStringLiteral("en"), QStringLiteral("fr"), QStringLiteral("es"),
    QStringLiteral("it"), QStringLiteral("nl"), QStringLiteral("pt"), QStringLiteral("sv"),
    QStringLiteral("da"), QStringLiteral("nb"), QStringLiteral("pl"), QStringLiteral("cs"),
    QStringLiteral("ru"), QStringLiteral("ja"), QStringLiteral("zh"), QStringLiteral("la"),
};

QStringList cleanTags(const QStringList &tags)
{
    QStringList result;
    for (const QString &tag : tags) {
        const QString clean = tag.simplified();
        if (!clean.isEmpty() && !result.contains(clean, Qt::CaseInsensitive))
            result.append(clean);
    }
    return result;
}

QString formatValue(const Work &work, Format::Kind kind, QString Format::*field)
{
    const Format *format = work.format(kind);
    return format ? format->*field : QString();
}

// Writes the value to all formats of this kind.
void setFormatValue(Work &work, Format::Kind kind, QString Format::*field, const QString &value)
{
    for (Format &format : work.formats) {
        if (format.kind == kind)
            format.*field = value;
    }
}

// A cover counts as square like on import (see ImportWorker).
bool isSquare(const QImage &image)
{
    const double ratio = double(image.width()) / std::max(1, image.height());
    return ratio > 0.9 && ratio < 1.1;
}

QUrl urlOfPath(const QString &path)
{
    if (path.isEmpty())
        return QUrl();
    return path.startsWith(u':') ? QUrl(QStringLiteral("qrc") + path) : QUrl::fromLocalFile(path);
}

QImage readImage(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true); // photos turned by their EXIF data
    return reader.read();
}

// The picture of a local file URL, null if it is none.
QImage imageOfUrl(const QUrl &url)
{
    const QString path = url.isLocalFile() ? url.toLocalFile() : QString();
    return path.isEmpty() ? QImage() : readImage(path);
}

} // namespace

WorkEditor::WorkEditor(QObject *parent)
    : QObject(parent)
{
}

WorkEditor::~WorkEditor() = default;

WorkEditor::Fields WorkEditor::Fields::of(const Work &work)
{
    Fields fields;
    fields.title = work.title;
    fields.authors = work.authors.join(QStringLiteral(", "));
    fields.series = work.series;
    fields.seriesIndex = work.seriesIndex;
    fields.language = work.language;
    fields.publishedYear = work.publishedYear;
    fields.tags = work.tags;
    fields.rating = work.rating;
    fields.description = work.description;
    fields.narrators = work.narrators.join(QStringLiteral(", "));
    fields.audiobookPublisher = formatValue(work, Format::Kind::Audiobook, &Format::publisher);
    fields.audiobookIsbn = formatValue(work, Format::Kind::Audiobook, &Format::isbn);
    fields.ebookPublisher = formatValue(work, Format::Kind::Ebook, &Format::publisher);
    fields.ebookIsbn = formatValue(work, Format::Kind::Ebook, &Format::isbn);
    return fields;
}

bool WorkEditor::Fields::sameAs(const Fields &other) const
{
    const auto same = [](const QString &a, const QString &b) { return a.trimmed() == b.trimmed(); };
    return same(title, other.title)
        && TextUtils::splitNames(authors) == TextUtils::splitNames(other.authors)
        && same(series, other.series)
        && seriesIndex == other.seriesIndex
        && language == other.language
        && publishedYear == other.publishedYear
        && cleanTags(tags) == cleanTags(other.tags)
        && rating == other.rating
        && same(description, other.description)
        && TextUtils::splitNames(narrators) == TextUtils::splitNames(other.narrators)
        && same(audiobookPublisher, other.audiobookPublisher)
        && same(audiobookIsbn, other.audiobookIsbn)
        && same(ebookPublisher, other.ebookPublisher)
        && same(ebookIsbn, other.ebookIsbn);
}

void WorkEditor::setWorkId(const QString &workId)
{
    if (workId == m_workId)
        return;
    m_workId = workId;
    emit workIdChanged();

    const LibraryController *library = LibraryController::instance();
    const WorkModel *works = library ? library->works() : nullptr;
    const Work *work = works ? works->workAt(works->rowOf(workId)) : nullptr;
    m_valid = work != nullptr;
    load(work ? *work : Work());
}

void WorkEditor::load(const Work &work)
{
    m_original = work;
    m_originalFields = Fields::of(work);
    m_fields = m_originalFields;
    clearCovers();
    emit loaded();
    emit fieldsChanged();
    updateModified();
}

bool WorkEditor::canSave() const
{
    return m_valid && m_modified && !m_fields.title.trimmed().isEmpty();
}

QVariantList WorkEditor::languages() const
{
    QStringList codes = kCommonLanguages;
    if (!m_original.language.isEmpty() && !codes.contains(m_original.language))
        codes.append(m_original.language);

    QList<QPair<QString, QString>> entries; // name, code
    for (const QString &code : std::as_const(codes))
        entries.append({ TextUtils::languageName(code), code });
    QCollator collator;
    std::sort(entries.begin(), entries.end(), [&collator](const auto &a, const auto &b) {
        return collator.compare(a.first, b.first) < 0;
    });

    QVariantList result;
    for (const auto &entry : std::as_const(entries))
        result.append(QVariantMap{ { QStringLiteral("code"), entry.second }, { QStringLiteral("name"), entry.first } });
    return result;
}

template <typename T>
void WorkEditor::setField(T Fields::*field, const T &value)
{
    if (m_fields.*field == value)
        return;
    m_fields.*field = value;
    emit fieldsChanged();
    updateModified();
}

void WorkEditor::setTitle(const QString &value) { setField(&Fields::title, value); }
void WorkEditor::setAuthors(const QString &value) { setField(&Fields::authors, value); }
void WorkEditor::setSeries(const QString &value) { setField(&Fields::series, value); }
void WorkEditor::setSeriesIndex(int value) { setField(&Fields::seriesIndex, std::max(0, value)); }
void WorkEditor::setLanguage(const QString &value) { setField(&Fields::language, value.trimmed()); }
void WorkEditor::setPublishedYear(int value) { setField(&Fields::publishedYear, std::max(0, value)); }
void WorkEditor::setTags(const QStringList &value) { setField(&Fields::tags, cleanTags(value)); }
void WorkEditor::setRating(int value) { setField(&Fields::rating, std::clamp(value, 0, 5)); }
void WorkEditor::setDescription(const QString &value) { setField(&Fields::description, value); }
void WorkEditor::setNarrators(const QString &value) { setField(&Fields::narrators, value); }
void WorkEditor::setAudiobookPublisher(const QString &value) { setField(&Fields::audiobookPublisher, value); }
void WorkEditor::setAudiobookIsbn(const QString &value) { setField(&Fields::audiobookIsbn, value); }
void WorkEditor::setEbookPublisher(const QString &value) { setField(&Fields::ebookPublisher, value); }
void WorkEditor::setEbookIsbn(const QString &value) { setField(&Fields::ebookIsbn, value); }

void WorkEditor::updateModified()
{
    const bool modified = m_valid && (!m_fields.sameAs(m_originalFields) || coverModified());
    if (modified == m_modified)
        return;
    m_modified = modified;
    emit modifiedChanged();
}

Work WorkEditor::applied(Work work) const
{
    const Fields &f = m_fields;
    const Fields &o = m_originalFields;
    work.title = f.title.trimmed();
    work.authors = TextUtils::splitNames(f.authors);
    work.series = f.series.trimmed();
    work.seriesIndex = work.series.isEmpty() ? 0 : f.seriesIndex;
    work.language = f.language;
    work.publishedYear = f.publishedYear;
    work.tags = cleanTags(f.tags);
    work.rating = f.rating;
    work.description = f.description.trimmed();
    work.narrators = TextUtils::splitNames(f.narrators);

    // Format fields only when changed, so formats of the same kind keep
    // their own values otherwise.
    const auto apply = [&work](Format::Kind kind, QString Format::*field, const QString &value,
                               const QString &original) {
        if (value.trimmed() != original.trimmed())
            setFormatValue(work, kind, field, value.trimmed());
    };
    apply(Format::Kind::Audiobook, &Format::publisher, f.audiobookPublisher, o.audiobookPublisher);
    apply(Format::Kind::Audiobook, &Format::isbn, f.audiobookIsbn, o.audiobookIsbn);
    apply(Format::Kind::Ebook, &Format::publisher, f.ebookPublisher, o.ebookPublisher);
    apply(Format::Kind::Ebook, &Format::isbn, f.ebookIsbn, o.ebookIsbn);
    return work;
}

bool WorkEditor::save()
{
    if (!m_modified)
        return m_valid;
    if (!canSave())
        return false;
    LibraryController *library = LibraryController::instance();
    const Work *current = library ? library->works()->workAt(library->works()->rowOf(m_workId)) : nullptr;
    if (!current)
        return false;

    // Start from the current state, so e.g. progress saved meanwhile is kept.
    Work work = applied(*current);
    QStringList written;
    QStringList replaced;
    const bool stored = applyCovers(work, &written, &replaced) && library->saveWork(work);
    // The covers that are not used (any more) go.
    for (const QString &path : stored ? replaced : written)
        QFile::remove(path);
    if (!stored)
        return false;
    load(work);
    return true;
}

void WorkEditor::revert()
{
    m_fields = m_originalFields;
    clearCovers();
    emit fieldsChanged();
    updateModified();
}

// --- covers -------------------------------------------------------------------

QList<Format::Kind> WorkEditor::kindsOf(CoverTarget target) const
{
    QList<Format::Kind> kinds;
    if (target != AudiobookCover && m_original.hasEbook())
        kinds.append(Format::Kind::Ebook);
    if (target != EbookCover && m_original.hasAudiobook())
        kinds.append(Format::Kind::Audiobook);
    return kinds;
}

QUrl WorkEditor::coverUrl(Format::Kind kind) const
{
    const PendingCover &cover = pending(kind);
    if (cover.changed)
        return urlOfPath(cover.previewPath);
    return urlOfPath(m_original.coverPath(kind));
}

bool WorkEditor::coverIsSquare(Format::Kind kind) const
{
    const PendingCover &cover = pending(kind);
    if (!cover.changed)
        return m_original.coverIsSquare(kind);
    // No cover: the placeholder of an audiobook is square (see Work::coverIsSquare()).
    return cover.image.isNull() ? kind == Format::Kind::Audiobook : isSquare(cover.image);
}

QImage WorkEditor::currentCover(Format::Kind kind) const
{
    const PendingCover &cover = pending(kind);
    if (cover.changed)
        return cover.image;
    const QString path = m_original.coverPath(kind);
    return path.isEmpty() ? QImage() : readImage(path);
}

void WorkEditor::clearCovers()
{
    const bool had = coverModified();
    m_ebookCover = PendingCover();
    m_audiobookCover = PendingCover();
    m_previewDir.reset();
    if (had)
        emit coverChanged();
}

void WorkEditor::setCover(CoverTarget target, const QImage &image)
{
    const QList<Format::Kind> kinds = kindsOf(target);
    if (!m_valid || kinds.isEmpty())
        return;
    // The view shows the picture from a file: a new name each time, so it
    // does not keep showing an older one from its image cache.
    QString previewPath;
    if (!image.isNull()) {
        if (!m_previewDir)
            m_previewDir = std::make_unique<QTemporaryDir>();
        previewPath = m_previewDir->isValid()
                          ? m_previewDir->filePath(QStringLiteral("cover-%1.png").arg(++m_previewNumber)) : QString();
        if (previewPath.isEmpty() || !image.save(previewPath, "PNG"))
            previewPath.clear();
    }
    for (const Format::Kind kind : kinds)
        pending(kind) = PendingCover{ true, image, previewPath };
    emit coverChanged();
    updateModified();
}

bool WorkEditor::setCoverFromUrl(CoverTarget target, const QUrl &url)
{
    const QImage image = imageOfUrl(url);
    if (image.isNull())
        return false;
    setCover(target, image);
    return true;
}

bool WorkEditor::canPasteCover() const
{
    const QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *data = clipboard ? clipboard->mimeData() : nullptr;
    if (!data)
        return false;
    if (data->hasImage())
        return true;
    // A picture file copied in the Explorer.
    const QList<QUrl> urls = data->urls();
    return urls.size() == 1 && urls.first().isLocalFile()
        && !QImageReader::imageFormat(urls.first().toLocalFile()).isEmpty();
}

bool WorkEditor::pasteCover(CoverTarget target)
{
    const QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *data = clipboard ? clipboard->mimeData() : nullptr;
    if (!data)
        return false;
    QImage image;
    if (data->hasImage())
        image = qvariant_cast<QImage>(data->imageData());
    if (image.isNull() && data->urls().size() == 1)
        image = imageOfUrl(data->urls().first());
    if (image.isNull())
        return false;
    setCover(target, image);
    return true;
}

bool WorkEditor::coverFromFile(CoverTarget target)
{
    // Each format kind from its own file, so "Allgemein" gives both back.
    bool found = false;
    for (const Format::Kind kind : kindsOf(target)) {
        const Format *format = m_original.format(kind);
        if (!format || format->filePath.isEmpty() || !QFileInfo::exists(format->filePath))
            continue;
        const QImage image = readMetadata(format->filePath).cover;
        if (image.isNull())
            continue;
        setCover(kind == Format::Kind::Ebook ? EbookCover : AudiobookCover, image);
        found = true;
    }
    return found;
}

bool WorkEditor::canUseOtherCover(CoverTarget target) const
{
    if (target == AllFormats || kindsOf(target).isEmpty())
        return false;
    const Format::Kind other = target == EbookCover ? Format::Kind::Audiobook : Format::Kind::Ebook;
    if (!m_original.hasFormat(other))
        return false;
    const PendingCover &cover = pending(other);
    return cover.changed ? !cover.image.isNull() : !m_original.coverPath(other).isEmpty();
}

bool WorkEditor::useOtherCover(CoverTarget target)
{
    if (!canUseOtherCover(target))
        return false;
    const QImage image = currentCover(target == EbookCover ? Format::Kind::Audiobook : Format::Kind::Ebook);
    if (image.isNull())
        return false;
    setCover(target, image);
    return true;
}

void WorkEditor::removeCover(CoverTarget target)
{
    setCover(target, QImage());
}

bool WorkEditor::applyCovers(Work &work, QStringList *written, QStringList *replaced) const
{
    for (const Format::Kind kind : { Format::Kind::Ebook, Format::Kind::Audiobook }) {
        const PendingCover &cover = pending(kind);
        if (!cover.changed)
            continue;
        for (int i = 0; i < work.formats.size(); ++i) {
            Format &format = work.formats[i];
            if (format.kind != kind)
                continue;
            // A combined work: only the volume that gives the work its cover.
            if (work.isCombined() && i != work.formatIndex(kind))
                continue;
            const QString old = format.coverPath;
            if (cover.image.isNull()) {
                format.coverPath.clear();
                format.coverIsSquare = false;
            } else {
                const QString path = LibraryFolder::saveNewCover(cover.image, format.filePath, old);
                if (path.isEmpty()) {
                    qWarning("Cover: cannot save next to %s", qPrintable(format.filePath));
                    return false;
                }
                written->append(path);
                format.coverPath = path;
                format.coverIsSquare = isSquare(cover.image);
            }
            if (!old.isEmpty() && LibraryFolder::isOwnCover(old, format.filePath))
                replaced->append(old);
        }
    }
    return true;
}
