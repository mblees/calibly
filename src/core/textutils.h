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

// Sort key of a person's name: the last word, e.g.
// "Johann Wolfgang von Goethe" -> "Goethe", "E. T. A. Hoffmann" -> "Hoffmann".
QString nameSortKey(const QString &name);

} // namespace TextUtils
