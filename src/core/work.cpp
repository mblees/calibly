// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "work.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <utility>

const Format *Work::format(Format::Kind kind) const
{
    const int index = formatIndex(kind);
    return index < 0 ? nullptr : &formats.at(index);
}

int Work::formatIndex(Format::Kind kind) const
{
    for (int i = 0; i < formats.size(); ++i) {
        if (formats.at(i).kind == kind)
            return i;
    }
    return -1;
}

Format *Work::format(Format::Kind kind)
{
    return const_cast<Format *>(std::as_const(*this).format(kind));
}

double Work::progress(Format::Kind kind) const
{
    double sum = 0.0;
    double weightedSum = 0.0;
    double highest = 0.0;
    qint64 pages = 0;
    int count = 0;
    bool allPages = true;
    for (const Format &format : formats) {
        if (format.kind != kind)
            continue;
        ++count;
        highest = std::max(highest, format.progress);
        sum += format.progress;
        weightedSum += format.progress * format.pages;
        pages += format.pages;
        allPages = allPages && format.pages > 0;
    }
    if (count == 0)
        return 0.0;
    // Several formats of a work that was not combined are the same book
    // (e.g. EPUB and PDF): the furthest one counts.
    if (!isCombined())
        return highest;
    if (count > 1 && allPages && pages > 0)
        return weightedSum / double(pages);
    return sum / count;
}

int Work::formatCount(Format::Kind kind) const
{
    return int(std::count_if(formats.cbegin(), formats.cend(), [kind](const Format &f) { return f.kind == kind; }));
}

int Work::currentFormat(Format::Kind kind) const
{
    QList<int> indices;
    int recent = -1;
    for (int i = 0; i < formats.size(); ++i) {
        if (formats.at(i).kind != kind)
            continue;
        indices.append(i);
        const QDateTime &opened = formats.at(i).lastOpened;
        if (opened.isValid() && (recent < 0 || opened > formats.at(recent).lastOpened))
            recent = i;
    }
    if (indices.isEmpty())
        return -1;
    const auto firstUnfinished = [&](qsizetype from) {
        for (qsizetype k = from; k < indices.size(); ++k) {
            if (!formats.at(indices.at(k)).finished())
                return indices.at(k);
        }
        return -1;
    };
    if (recent >= 0) {
        if (!formats.at(recent).finished())
            return recent;
        if (const int next = firstUnfinished(indices.indexOf(recent) + 1); next >= 0)
            return next;
    }
    const int unfinished = firstUnfinished(0);
    return unfinished >= 0 ? unfinished : indices.first();
}

int Work::partOf(const QString &filePath) const
{
    if (filePath.isEmpty())
        return -1;
    const QString path = QDir::cleanPath(filePath);
    for (int i = 0; i < parts.size(); ++i) {
        for (const QString &file : parts.at(i).files) {
            if (QDir::cleanPath(file).compare(path, Qt::CaseInsensitive) == 0)
                return i;
        }
    }
    return -1;
}

QString Work::formatTitle(int formatIndex) const
{
    if (formatIndex < 0 || formatIndex >= formats.size())
        return QString();
    const QString &path = formats.at(formatIndex).filePath;
    const int part = partOf(path);
    if (part >= 0 && !parts.at(part).title.isEmpty())
        return parts.at(part).title;
    return QFileInfo(path).completeBaseName();
}

QString Work::coverPath(Format::Kind kind) const
{
    const Format *f = format(kind);
    return f ? f->coverPath : QString();
}

bool Work::coverIsSquare(Format::Kind kind) const
{
    const Format *f = format(kind);
    if (f && !f->coverPath.isEmpty())
        return f->coverIsSquare;
    return kind == Format::Kind::Audiobook;
}

namespace {

// kind == nullptr: all formats.
Work::State stateOf(const Work &work, const Format::Kind *kind)
{
    bool missing = false;
    bool finished = false;
    bool started = work.lastOpened.isValid();
    for (const Format &format : work.formats) {
        started = started || format.progress > 0.0 || format.lastOpened.isValid();
        if (kind && format.kind != *kind)
            continue;
        missing = missing || format.fileMissing;
    }
    // Volumes of a combined work: a kind is finished when all its formats
    // are. Otherwise several formats are the same book: one is enough.
    for (const Format::Kind k : { Format::Kind::Ebook, Format::Kind::Audiobook }) {
        if (kind && k != *kind)
            continue;
        bool any = false;
        bool all = true;
        for (const Format &format : work.formats) {
            if (format.kind != k)
                continue;
            any = any || (!work.isCombined() && format.finished());
            all = all && format.finished();
            if (work.isCombined())
                any = true;
        }
        finished = finished || (work.isCombined() ? any && all : any);
    }
    if (missing)
        return Work::State::FileMissing;
    if (finished)
        return Work::State::Finished;
    return started ? Work::State::Normal : Work::State::New;
}

} // namespace

Work::State Work::state() const
{
    return stateOf(*this, nullptr);
}

Work::State Work::state(Format::Kind kind) const
{
    return stateOf(*this, &kind);
}

void Work::markOpened(int formatIndex, const QDateTime &when)
{
    lastOpened = when;
    if (formatIndex >= 0 && formatIndex < formats.size())
        formats[formatIndex].lastOpened = when;
}

void Work::inheritLastOpened()
{
    if (!lastOpened.isValid() || formats.isEmpty())
        return;
    const bool known = std::any_of(formats.cbegin(), formats.cend(),
                                   [](const Format &format) { return format.lastOpened.isValid(); });
    if (known)
        return;
    bool any = false;
    for (Format &format : formats) {
        if (format.progress > 0.0 || format.positionMs > 0) {
            format.lastOpened = lastOpened;
            any = true;
        }
    }
    if (!any)
        formats.first().lastOpened = lastOpened;
}
