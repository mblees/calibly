#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <memory>

#include "booksource.h"

class QZipReader;

// An EPUB (2 or 3) opened for reading: the chapters in the order of the
// spine (only the linear ones), their files, the table of contents from the
// EPUB 3 navigation document or, without one, from the EPUB 2 NCX, and the
// font files of the book.
class EpubBook : public BookSource
{
public:
    EpubBook();
    ~EpubBook() override;

    // False (with a reason) if the file is no readable EPUB or has no chapters.
    bool open(const QString &filePath, QString *error = nullptr);

    QString title() const override { return m_title; }
    int chapterCount() const override { return int(m_chapters.size()); }
    QString chapterText(int chapter) const override;
    QString chapterPath(int chapter) const override;
    QByteArray resource(const QString &path) const override;
    QList<ContentsEntry> contents() const override { return m_contents; }
    QList<QByteArray> fonts() const override;
    qint64 chapterWeight(int chapter) const override;

private:
    struct ManifestItem
    {
        QString path;       // inside the ZIP
        QString mediaType;
        QString properties;
    };

    QByteArray file(const QString &path) const;
    void readContents(const QHash<QString, ManifestItem> &manifest, const QString &navId, const QString &ncxId);
    void readNav(const QString &navPath);
    void readNcx(const QString &ncxPath);
    void addContentsEntry(const QString &title, int level, const QString &basePath, const QString &href);

    std::unique_ptr<QZipReader> m_zip;
    QHash<QString, QString> m_namesByLowerCase; // ZIP entries, for links with other letter case
    QString m_title;
    QStringList m_chapters;                     // paths, in reading order
    QList<qint64> m_weights;
    QList<ContentsEntry> m_contents;
    QStringList m_fontPaths;
};

// Decodes the bytes of an (X)HTML or text file: a byte order mark or the
// encoding in the XML declaration, otherwise UTF-8.
QString decodeText(const QByteArray &data);
