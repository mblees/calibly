#pragma once

#include <QList>

#include <functional>

#include "work.h"

// "Kombinieren" (phase 9: "Zu einem Werk verbinden"): joins several works
// into one (pure logic, no files or databases are touched); "Kombination
// auflösen" gives them back (phase 12).
//
// The result keeps the id and the metadata of the target: the first work
// that has an ebook (as in the import, ebook metadata wins), otherwise the
// first work. Empty fields are filled from the other works, lists (narrators,
// tags) are united, the rating is the highest one, "added" the earliest and
// "last opened" the latest date. The formats of all works are kept; several
// audiobook formats become one, see joinAudiobooks().
// The result remembers the works it was made of (Work::parts; a combined
// work passes on its parts), ordered as volumes: by series number, then by
// title ("Band 2" before "Band 10"). The ebook formats follow that order.
// An empty list gives a work without id.
Work mergeWorks(const QList<Work> &works);

// Index in works of the work that mergeWorks() takes as the target, -1 for
// an empty list.
int mergeTarget(const QList<Work> &works);

// Makes one audiobook out of all audiobook formats of the work, e.g. the MP3
// files of a folder that were imported one by one: the files follow each
// other in the order of the work's parts, then of their paths ("Teil 2"
// before "Teil 10"), each keeping its chapters (a file without chapters
// becomes one chapter). The listening position is where the last file with
// a position was stopped.
// Does nothing for works with less than two audiobook formats; returns
// whether the work was changed.
bool joinAudiobooks(Work &work);

// What a work stands for as a part of a combined work: its metadata and
// the files of its formats.
WorkPart partFromWork(const Work &work);

// "Kombination auflösen": the works a work was combined from.
// With stored parts every part comes back with its id and metadata; the
// part with the id of the work keeps the metadata of the work (changes made
// to the combined entry). Each format goes to the part its file belongs to
// (a file of no part to that part as well). A joined audiobook is cut into
// the files of each part; the listening position goes to the part it is in,
// the parts before it count as finished, the parts after it as not started.
// Without stored parts (merged before phase 12) every format becomes a work
// of its own with the metadata of the work and the name of its file as
// title; the first keeps the id. newId() gives the ids of new works.
// A work that cannot be split comes back unchanged.
QList<Work> splitWork(const Work &work, const std::function<QString()> &newId);
