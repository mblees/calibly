#pragma once

#include <QList>

#include "work.h"

// "Zu einem Werk verbinden": joins several works into one (pure logic, no
// files or databases are touched).
//
// The result keeps the id and the metadata of the target: the first work
// that has an ebook (as in the import, ebook metadata wins), otherwise the
// first work. Empty fields are filled from the other works, lists (narrators,
// tags) are united, the rating is the highest one, "added" the earliest and
// "last opened" the latest date. The formats of all works are kept; several
// audiobook formats become one, see joinAudiobooks().
// An empty list gives a work without id.
Work mergeWorks(const QList<Work> &works);

// Index in works of the work that mergeWorks() takes as the target, -1 for
// an empty list.
int mergeTarget(const QList<Work> &works);

// Makes one audiobook out of all audiobook formats of the work, e.g. the MP3
// files of a folder that were imported one by one: the files follow each
// other in the order of their paths ("Teil 2" before "Teil 10"), each
// keeping its chapters (a file without chapters becomes one chapter). The
// listening position is where the last file with a position was stopped.
// Does nothing for works with less than two audiobook formats; returns
// whether the work was changed.
bool joinAudiobooks(Work &work);
