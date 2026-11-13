// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>
#include <QStringList>

namespace TextUtils {

// Lower case without accents, for search: "Märchen" -> "marchen",
// "Büchner" -> "buchner". Both the searched text and the search terms are
// folded, so "marchen" and "märchen" find the same works.
QString foldForSearch(const QString &text);

// Splits a search string into folded terms; empty terms are dropped.
QStringList searchTerms(const QString &text);

// Name of a language in that language, e.g. "de" -> "Deutsch", "en" -> "English".
// Empty code gives an empty string.
QString languageName(const QString &code);

// Splits a list of names such as "Jacob Grimm, Wilhelm Grimm",
// "A; B" or "A & B" into the single names, trimmed, without empty ones.
QStringList splitNames(const QString &names);

// Sort key of a person's name: the last word, e.g.
// "Johann Wolfgang von Goethe" -> "Goethe", "E. T. A. Hoffmann" -> "Hoffmann".
QString nameSortKey(const QString &name);

} // namespace TextUtils
