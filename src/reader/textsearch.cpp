// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "textsearch.h"

#include <algorithm>

namespace TextSearch {

namespace {

constexpr char16_t kSoftHyphen = 0x00AD;

bool isSpace(QChar c)
{
    // QChar::isSpace() covers spaces, tabs, line and paragraph separators;
    // QTextDocument also uses U+FDD0 / U+FDD1 around frames (e.g. tables).
    return c.isSpace() || c.unicode() == 0xFDD0 || c.unicode() == 0xFDD1 || c == QChar::ObjectReplacementCharacter;
}

// The character as it is compared.
QChar folded(QChar c)
{
    return isSpace(c) ? QChar(u' ') : c.toCaseFolded();
}

// A character of the text that is not written in the excerpt as it is.
QChar shown(QChar c)
{
    return isSpace(c) ? QChar(u' ') : c;
}

// Length of a match of the query starting at `start`, 0 if there is none.
int matchAt(const QString &text, int start, const QString &query)
{
    int i = start;
    for (int q = 0; q < query.size(); ++q) {
        const QChar wanted = query.at(q);
        if (wanted == u' ') {
            // One or more spaces of any kind.
            if (i >= text.size() || !isSpace(text.at(i)))
                return 0;
            while (i < text.size() && (isSpace(text.at(i)) || text.at(i) == kSoftHyphen))
                ++i;
            continue;
        }
        while (i < text.size() && text.at(i) == kSoftHyphen && i > start)
            ++i;
        if (i >= text.size() || folded(text.at(i)) != wanted)
            return 0;
        ++i;
    }
    return i - start;
}

// One line of the excerpt: spaces of every kind made one space.
QString oneLine(const QString &text)
{
    QString result;
    result.reserve(text.size());
    for (const QChar c : text) {
        if (c == kSoftHyphen)
            continue;
        const QChar s = shown(c);
        if (s == u' ' && result.endsWith(u' '))
            continue;
        result.append(s);
    }
    return result;
}

} // namespace

QString normalizedQuery(const QString &query)
{
    QString result;
    for (const QChar c : query.simplified()) {
        if (c != kSoftHyphen)
            result.append(folded(c));
    }
    return result;
}

QList<Match> findAll(const QString &text, const QString &query, int maxResults)
{
    QList<Match> matches;
    const QString wanted = normalizedQuery(query);
    if (wanted.isEmpty() || maxResults <= 0)
        return matches;
    const QChar first = wanted.at(0);
    for (int i = 0; i < text.size();) {
        if (folded(text.at(i)) == first) {
            const int length = matchAt(text, i, wanted);
            if (length > 0) {
                matches.append({ i, length });
                if (matches.size() >= maxResults)
                    break;
                i += length;
                continue;
            }
        }
        ++i;
    }
    return matches;
}

Excerpt excerpt(const QString &text, const Match &match, int context)
{
    const int start = std::clamp(match.offset, 0, int(text.size()));
    const int end = std::clamp(match.offset + match.length, start, int(text.size()));

    // Back to a word border, at most about `context` characters.
    int from = std::max(0, start - context);
    if (from > 0) {
        const int space = text.indexOf(u' ', from);
        if (space >= 0 && space < start)
            from = space + 1;
    }
    int to = std::min(int(text.size()), end + context);
    if (to < text.size()) {
        const int space = text.lastIndexOf(u' ', to);
        if (space > end)
            to = space;
    }

    Excerpt result;
    // A paragraph break shows as a space: none at the outer ends.
    result.before = oneLine(text.mid(from, start - from));
    while (result.before.startsWith(u' '))
        result.before.remove(0, 1);
    result.match = oneLine(text.mid(start, end - start));
    result.after = oneLine(text.mid(end, to - end));
    while (result.after.endsWith(u' '))
        result.after.chop(1);
    if (from > 0)
        result.before.prepend(QStringLiteral("…"));
    if (to < text.size())
        result.after.append(QStringLiteral("…"));
    return result;
}

} // namespace TextSearch
