// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "pdfreader.h"

#ifdef CALIBLY_HAS_QTPDF
#include <QPainter>
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
            // Most PDFs draw no page background: the rendered page is
            // transparent there and would turn black as a JPEG cover.
            QImage cover(size, QImage::Format_RGB32);
            cover.fill(Qt::white);
            QPainter painter(&cover);
            painter.drawImage(0, 0, document.render(0, size));
            painter.end();
            metadata.cover = cover;
        }
    }
#else
    Q_UNUSED(path)
    Q_UNUSED(metadata)
#endif
}
