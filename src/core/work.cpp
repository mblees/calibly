#include "work.h"

#include <algorithm>
#include <utility>

const Format *Work::format(Format::Kind kind) const
{
    const auto it = std::find_if(formats.cbegin(), formats.cend(),
                                 [kind](const Format &format) { return format.kind == kind; });
    return it == formats.cend() ? nullptr : &*it;
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
