// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "workeditor.h"

#include <QCollator>
#include <QVariantMap>

#include <algorithm>

#include "librarycontroller.h"
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

} // namespace

WorkEditor::WorkEditor(QObject *parent)
    : QObject(parent)
{
}

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
    const bool modified = m_valid && !m_fields.sameAs(m_originalFields);
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
    const Work work = applied(*current);
    if (!library->saveWork(work))
        return false;
    load(work);
    return true;
}

void WorkEditor::revert()
{
    m_fields = m_originalFields;
    emit fieldsChanged();
    updateModified();
}
