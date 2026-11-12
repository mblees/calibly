#include "htmlprep.h"

#include <QRegularExpression>
#include <QStringList>

namespace HtmlPrep {

namespace {

constexpr auto kOptions = QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption;

// Replaces every match; replacement(match) gives the new text.
QString replaceAll(const QString &text, const QRegularExpression &pattern,
                   const std::function<QString(const QRegularExpressionMatch &)> &replacement)
{
    QString result;
    qsizetype last = 0;
    QRegularExpressionMatchIterator it = pattern.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        result += QStringView(text).mid(last, match.capturedStart() - last);
        result += replacement(match);
        last = match.capturedEnd();
    }
    result += QStringView(text).mid(last);
    return result;
}

} // namespace

QString cleanDeclarations(const QString &declarations, bool keepFonts, int emPixels)
{
    static const QRegularExpression lengths(QStringLiteral("(-?\\d*\\.?\\d+)\\s*(rem|em|pt)\\b"),
                                            QRegularExpression::CaseInsensitiveOption);
    static const QStringList dropped = {
        QStringLiteral("color"), QStringLiteral("background"), QStringLiteral("background-color"),
        QStringLiteral("background-image"), QStringLiteral("line-height"), QStringLiteral("font-size"),
        QStringLiteral("font"), QStringLiteral("letter-spacing"), QStringLiteral("word-spacing"),
    };
    QStringList kept;
    for (const QString &declaration : declarations.split(u';', Qt::SkipEmptyParts)) {
        const QString property = declaration.section(u':', 0, 0).trimmed().toLower();
        if (property.isEmpty() || !declaration.contains(u':'))
            continue;
        if (dropped.contains(property) || (!keepFonts && property == QLatin1String("font-family")))
            continue;
        QString value = declaration.trimmed();
        if (emPixels > 0) {
            value = replaceAll(value, lengths, [emPixels](const QRegularExpressionMatch &match) {
                const double number = match.captured(1).toDouble();
                const double factor = match.captured(2).compare(QLatin1String("pt"), Qt::CaseInsensitive) == 0
                                          ? 4.0 / 3.0 : double(emPixels);
                return QString::number(qRound(number * factor)) + QStringLiteral("px");
            });
        }
        kept.append(value);
    }
    return kept.join(QStringLiteral("; "));
}

QString cleanCss(const QString &css, bool keepFonts, int emPixels)
{
    static const QRegularExpression comments(QStringLiteral("/\\*.*?\\*/"), kOptions);
    static const QRegularExpression atRules(QStringLiteral("@(font-face|page)\\s*\\{[^}]*\\}"), kOptions);
    static const QRegularExpression imports(QStringLiteral("@import[^;]*;"), kOptions);
    static const QRegularExpression blocks(QStringLiteral("\\{([^{}]*)\\}"));
    QString result = css;
    result.remove(comments);
    result.remove(atRules);
    result.remove(imports);
    return replaceAll(result, blocks, [keepFonts, emPixels](const QRegularExpressionMatch &match) {
        return u'{' + cleanDeclarations(match.captured(1), keepFonts, emPixels) + u'}';
    });
}

QString prepare(const QString &html, bool keepFonts, int emPixels,
                const std::function<QString(const QString &href)> &stylesheet)
{
    static const QRegularExpression xmlDeclaration(QStringLiteral("<\\?xml[^>]*\\?>"), kOptions);
    static const QRegularExpression doctype(QStringLiteral("<!DOCTYPE[^>]*>"), kOptions);
    static const QRegularExpression links(QStringLiteral("<link\\b[^>]*>"), kOptions);
    static const QRegularExpression href(QStringLiteral("\\bhref\\s*=\\s*[\"']([^\"']+)[\"']"), kOptions);
    static const QRegularExpression styles(QStringLiteral("(<style\\b[^>]*>)(.*?)(</style>)"), kOptions);
    static const QRegularExpression styleAttributes(QStringLiteral("\\bstyle\\s*=\\s*(\"([^\"]*)\"|'([^']*)')"), kOptions);
    static const QRegularExpression svgs(QStringLiteral("<svg\\b.*?</svg>"), kOptions);
    static const QRegularExpression svgImage(QStringLiteral("<image\\b[^>]*?\\b(?:xlink:)?href\\s*=\\s*[\"']([^\"']+)[\"']"), kOptions);
    static const QRegularExpression anchorIds(QStringLiteral("<a\\b([^>]*?)\\sid\\s*=\\s*[\"']([^\"']+)[\"']"), kOptions);
    static const QRegularExpression elementIds(QStringLiteral("<(?!a\\b)([a-zA-Z][\\w:-]*)\\b[^>]*?\\sid\\s*=\\s*[\"']([^\"']+)[\"'][^>]*>"), kOptions);

    QString result = html;
    result.remove(xmlDeclaration);
    result.remove(doctype);

    result = replaceAll(result, links, [&](const QRegularExpressionMatch &match) {
        const QString tag = match.captured(0);
        if (!tag.contains(QLatin1String("stylesheet"), Qt::CaseInsensitive))
            return QString();
        const QRegularExpressionMatch target = href.match(tag);
        const QString css = target.hasMatch() && stylesheet ? stylesheet(target.captured(1)) : QString();
        return css.isEmpty() ? QString()
                             : QStringLiteral("<style>") + cleanCss(css, keepFonts, emPixels) + QStringLiteral("</style>");
    });
    result = replaceAll(result, styles, [keepFonts, emPixels](const QRegularExpressionMatch &match) {
        return match.captured(1) + cleanCss(match.captured(2), keepFonts, emPixels) + match.captured(3);
    });
    result = replaceAll(result, styleAttributes, [keepFonts, emPixels](const QRegularExpressionMatch &match) {
        const QString value = match.captured(2).isEmpty() ? match.captured(3) : match.captured(2);
        return QStringLiteral("style=\"") + cleanDeclarations(value, keepFonts, emPixels) + u'"';
    });
    result = replaceAll(result, svgs, [](const QRegularExpressionMatch &match) {
        const QRegularExpressionMatch image = svgImage.match(match.captured(0));
        return image.hasMatch() ? QStringLiteral("<img src=\"") + image.captured(1) + QStringLiteral("\" />") : QString();
    });
    result = replaceAll(result, anchorIds, [](const QRegularExpressionMatch &match) {
        return QStringLiteral("<a") + match.captured(1) + QStringLiteral(" name=\"") + match.captured(2) + u'"';
    });
    result = replaceAll(result, elementIds, [](const QRegularExpressionMatch &match) {
        static const QStringList outside = { QStringLiteral("table"), QStringLiteral("tbody"), QStringLiteral("thead"),
                                             QStringLiteral("tfoot"), QStringLiteral("tr"), QStringLiteral("ol"),
                                             QStringLiteral("ul"), QStringLiteral("dl"), QStringLiteral("colgroup"),
                                             QStringLiteral("col") };
        const QString tag = match.captured(1).toLower();
        const QString anchor = QStringLiteral("<a name=\"") + match.captured(2) + QStringLiteral("\"></a>");
        if (tag == QLatin1String("html") || tag == QLatin1String("head"))
            return match.captured(0);
        // Inside a table or list an anchor would add an empty cell or item: put it before.
        return outside.contains(tag) ? anchor + match.captured(0) : match.captured(0) + anchor;
    });
    return result;
}

} // namespace HtmlPrep
