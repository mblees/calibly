#include "work.h"

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
    double result = 0.0;
    for (const Format &format : formats) {
        if (format.kind == kind)
            result = std::max(result, format.progress);
    }
    return result;
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
        finished = finished || format.finished();
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
