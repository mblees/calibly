#include "textutils.h"

#include <QHash>
#include <QLocale>

namespace TextUtils {

QString foldForSearch(const QString &text)
{
    // Decompose ("ä" -> "a" + combining diaeresis) and drop the combining marks.
    const QString decomposed = text.normalized(QString::NormalizationForm_KD);
    QString folded;
    folded.reserve(decomposed.size());
    for (const QChar c : decomposed) {
        if (c.category() != QChar::Mark_NonSpacing)
            folded.append(c);
    }
    // "ß" has no decomposition; fold it like a search for "ss" would expect.
    folded.replace(QChar(0x00DF), QStringLiteral("ss"));
    return folded.toCaseFolded();
}

QStringList searchTerms(const QString &text)
{
    return foldForSearch(text).split(u' ', Qt::SkipEmptyParts);
}

QString languageName(const QString &code)
{
    if (code.isEmpty())
        return QString();
    // QLocale names the language of the default territory ("en" -> "American
    // English"); for plain language codes we want the plain language name.
    static const QHash<QString, QString> overrides = {
        { QStringLiteral("en"), QStringLiteral("English") },
        { QStringLiteral("pt"), QStringLiteral("Português") },
    };
    if (overrides.contains(code))
        return overrides.value(code);

    QString name = QLocale(code).nativeLanguageName();
    if (!name.isEmpty())
        name[0] = name[0].toUpper();
    return name.isEmpty() ? code : name;
}

QStringList splitNames(const QString &names)
{
    QStringList result;
    const QStringList parts = QString(names).replace(u';', u',').replace(QStringLiteral(" & "), QStringLiteral(","))
                                  .split(u',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString name = part.simplified();
        if (!name.isEmpty())
            result.append(name);
    }
    return result;
}

QString nameSortKey(const QString &name)
{
    const QStringList parts = name.split(u' ', Qt::SkipEmptyParts);
    return parts.isEmpty() ? QString() : parts.last();
}

} // namespace TextUtils
