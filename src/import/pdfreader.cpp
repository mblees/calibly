#include "pdfreader.h"

#ifdef CALIBLY_HAS_QTPDF
#include <QPdfDocument>
#endif

bool pdfSupportAvailable()
{
#ifdef CALIBLY_HAS_QTPDF
    return true;
#else
    return false;
#endif
}

void readPdf(const QString &path, FileMetadata &metadata)
{
#ifdef CALIBLY_HAS_QTPDF
    QPdfDocument document;
    if (document.load(path) != QPdfDocument::Error::None) {
        metadata.error = QStringLiteral("not a readable PDF file");
        return;
    }
    metadata.title = document.metaData(QPdfDocument::MetaDataField::Title).toString().simplified();
    const QString author = document.metaData(QPdfDocument::MetaDataField::Author).toString().simplified();
    if (!author.isEmpty())
        metadata.authors = { author };
    metadata.pages = document.pageCount();
    if (metadata.pages > 0) {
        // First page as cover, rendered at a typical cover width.
        constexpr int coverWidth = 800;
        const QSizeF pageSize = document.pagePointSize(0);
        if (pageSize.width() > 0) {
            const QSize size(coverWidth, int(coverWidth * pageSize.height() / pageSize.width()));
            metadata.cover = document.render(0, size);
        }
    }
#else
    Q_UNUSED(path)
    Q_UNUSED(metadata)
#endif
}
