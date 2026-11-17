// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <QString>

// Finding a text in the text of a chapter (search in the book, phase 14).
// No UI, no Qt GUI.
//
// The comparison ignores case, treats every kind of space and line break
// (also the paragraph separators of QTextDocument::toRawText()) as one space
// and skips soft hyphens in the text, so "Kapitän" finds "Kapi­tän".
// Accents count: "Bar" does not find "Bär".
namespace TextSearch {

struct Match
{
    int offset = 0;   // first character in the text
    int length = 0;   // characters in the text (with skipped soft hyphens)

    bool operator==(const Match &other) const { return offset == other.offset && length == other.length; }
};

// The query as it is compared: simplified; "" if there is nothing to find.
QString normalizedQuery(const QString &query);

// All places of the query in the text, in order, at most maxResults; they
// do not overlap.
QList<Match> findAll(const QString &text, const QString &query, int maxResults = 1000);

// The words around a match, for the list of results: the text before it,
// the match and the text after it, each on one line and shortened to about
// `context` characters at a word border ("…" where it was cut).
struct Excerpt
{
    QString before;
    QString match;
    QString after;
};
Excerpt excerpt(const QString &text, const Match &match, int context = 40);

} // namespace TextSearch
