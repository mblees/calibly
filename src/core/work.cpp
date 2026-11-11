#include "work.h"

#include <algorithm>

bool Work::hasFormat(Format::Kind kind) const
{
    return std::any_of(formats.cbegin(), formats.cend(),
                       [kind](const Format &format) { return format.kind == kind; });
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
