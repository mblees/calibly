// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QImage>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariantList>

#include <memory>

#include "work.h"

// Editable copy of the metadata of one work, for the metadata editor
// (view E). The QML form shows these properties and writes the user's input
// back; nothing reaches the library before save().
//
// In QML (import Calibly.Controllers):
//   WorkEditor { id: editor; workId: "moby-dick" }
//   InputField { text: editor.title; onTextEdited: editor.title = text }
//   AppButton { enabled: editor.canSave; onClicked: editor.save() }
//
// Shared fields apply to the whole work and so to all its formats:
//   title, authors, series, seriesIndex, language, publishedYear, tags,
//   rating, description
// Audiobook fields: narrators, audiobookPublisher, audiobookIsbn
// Ebook fields:     ebookPublisher, ebookIsbn
// A changed format field is written to every format of that kind (e.g. EPUB
// and PDF); unchanged ones keep their own values.
//
// authors and narrators are edited as one text, e.g. "Jacob Grimm, Wilhelm
// Grimm"; names are separated by "," ";" or " & " (see TextUtils::splitNames).
// Texts are kept exactly as typed while editing and trimmed when saved.
//
// Properties:
//   workId     the work to edit; setting it loads the work and drops unsaved changes
//   valid      the work exists
//   modified   a field differs from the stored work
//   canSave    modified and the title is not empty
//   hasEbook, hasAudiobook   which format fields apply
//   languages  [{ code, name }] of common languages, plus the work's own one,
//              sorted by name; "" (unknown) is not included
//
// Covers (phase 14): every format has its own cover. A cover change is made
// for a target – AllFormats (tab "Allgemein"), EbookCover or AudiobookCover –
// and, like the fields, only stored by save(). Of a combined work only the
// first format of the kind (the one the cover of the work comes from) gets
// it; the other volumes keep theirs. On save the image is written next to
// the format file under a new name (LibraryFolder::saveNewCover()) and the
// cover calibly had saved there before is deleted.
//   ebookCoverUrl, audiobookCoverUrl            the cover as it will be saved
//   ebookCoverIsSquare, audiobookCoverIsSquare  (a pending change included)
//   coverModified   a cover change is pending
//
// Functions:
//   save()     stores the changes in the library (the gallery and the detail
//              view update immediately); true if stored or nothing to store
//   revert()   drops unsaved changes
//   setCoverFromUrl(target, url)   a picture file, e.g. picked or dropped;
//                                  false if it is no picture
//   pasteCover(target)             the picture on the clipboard (or a picture
//                                  file copied in the Explorer); false if none
//   canPasteCover()                the clipboard holds one
//   coverFromFile(target)          the cover in the book / audiobook file again
//                                  (a PDF: its first page); false if it has none
//   useOtherCover(target)          the cover of the other format: EbookCover
//                                  takes the audiobook's, AudiobookCover the ebook's
//   canUseOtherCover(target)       the other format exists and has a cover
//   removeCover(target)            no cover: the generated placeholder
class WorkEditor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString workId READ workId WRITE setWorkId NOTIFY workIdChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY loaded)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY modifiedChanged)
    Q_PROPERTY(bool hasEbook READ hasEbook NOTIFY loaded)
    Q_PROPERTY(bool hasAudiobook READ hasAudiobook NOTIFY loaded)
    Q_PROPERTY(QVariantList languages READ languages NOTIFY loaded)
    Q_PROPERTY(QUrl ebookCoverUrl READ ebookCoverUrl NOTIFY coverChanged)
    Q_PROPERTY(bool ebookCoverIsSquare READ ebookCoverIsSquare NOTIFY coverChanged)
    Q_PROPERTY(QUrl audiobookCoverUrl READ audiobookCoverUrl NOTIFY coverChanged)
    Q_PROPERTY(bool audiobookCoverIsSquare READ audiobookCoverIsSquare NOTIFY coverChanged)
    Q_PROPERTY(bool coverModified READ coverModified NOTIFY coverChanged)

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY fieldsChanged)
    Q_PROPERTY(QString authors READ authors WRITE setAuthors NOTIFY fieldsChanged)
    Q_PROPERTY(QString series READ series WRITE setSeries NOTIFY fieldsChanged)
    Q_PROPERTY(int seriesIndex READ seriesIndex WRITE setSeriesIndex NOTIFY fieldsChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY fieldsChanged)
    Q_PROPERTY(int publishedYear READ publishedYear WRITE setPublishedYear NOTIFY fieldsChanged)
    Q_PROPERTY(QStringList tags READ tags WRITE setTags NOTIFY fieldsChanged)
    Q_PROPERTY(int rating READ rating WRITE setRating NOTIFY fieldsChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY fieldsChanged)
    Q_PROPERTY(QString narrators READ narrators WRITE setNarrators NOTIFY fieldsChanged)
    Q_PROPERTY(QString audiobookPublisher READ audiobookPublisher WRITE setAudiobookPublisher NOTIFY fieldsChanged)
    Q_PROPERTY(QString audiobookIsbn READ audiobookIsbn WRITE setAudiobookIsbn NOTIFY fieldsChanged)
    Q_PROPERTY(QString ebookPublisher READ ebookPublisher WRITE setEbookPublisher NOTIFY fieldsChanged)
    Q_PROPERTY(QString ebookIsbn READ ebookIsbn WRITE setEbookIsbn NOTIFY fieldsChanged)

public:
    enum CoverTarget { AllFormats, EbookCover, AudiobookCover };
    Q_ENUM(CoverTarget)

    explicit WorkEditor(QObject *parent = nullptr);
    ~WorkEditor() override;

    QString workId() const { return m_workId; }
    void setWorkId(const QString &workId);

    bool valid() const { return m_valid; }
    bool modified() const { return m_modified; }
    bool canSave() const;
    bool hasEbook() const { return m_original.hasEbook(); }
    bool hasAudiobook() const { return m_original.hasAudiobook(); }
    QVariantList languages() const;

    QString title() const { return m_fields.title; }
    void setTitle(const QString &value);
    QString authors() const { return m_fields.authors; }
    void setAuthors(const QString &value);
    QString series() const { return m_fields.series; }
    void setSeries(const QString &value);
    int seriesIndex() const { return m_fields.seriesIndex; }
    void setSeriesIndex(int value);
    QString language() const { return m_fields.language; }
    void setLanguage(const QString &value);
    int publishedYear() const { return m_fields.publishedYear; }
    void setPublishedYear(int value);
    QStringList tags() const { return m_fields.tags; }
    void setTags(const QStringList &value);
    int rating() const { return m_fields.rating; }
    void setRating(int value);
    QString description() const { return m_fields.description; }
    void setDescription(const QString &value);
    QString narrators() const { return m_fields.narrators; }
    void setNarrators(const QString &value);
    QString audiobookPublisher() const { return m_fields.audiobookPublisher; }
    void setAudiobookPublisher(const QString &value);
    QString audiobookIsbn() const { return m_fields.audiobookIsbn; }
    void setAudiobookIsbn(const QString &value);
    QString ebookPublisher() const { return m_fields.ebookPublisher; }
    void setEbookPublisher(const QString &value);
    QString ebookIsbn() const { return m_fields.ebookIsbn; }
    void setEbookIsbn(const QString &value);

    QUrl ebookCoverUrl() const { return coverUrl(Format::Kind::Ebook); }
    bool ebookCoverIsSquare() const { return coverIsSquare(Format::Kind::Ebook); }
    QUrl audiobookCoverUrl() const { return coverUrl(Format::Kind::Audiobook); }
    bool audiobookCoverIsSquare() const { return coverIsSquare(Format::Kind::Audiobook); }
    bool coverModified() const { return m_ebookCover.changed || m_audiobookCover.changed; }

    Q_INVOKABLE bool save();
    Q_INVOKABLE void revert();

    Q_INVOKABLE bool setCoverFromUrl(CoverTarget target, const QUrl &url);
    Q_INVOKABLE bool pasteCover(CoverTarget target);
    Q_INVOKABLE bool canPasteCover() const;
    Q_INVOKABLE bool coverFromFile(CoverTarget target);
    Q_INVOKABLE bool useOtherCover(CoverTarget target);
    Q_INVOKABLE bool canUseOtherCover(CoverTarget target) const;
    Q_INVOKABLE void removeCover(CoverTarget target);
    // The image as the cover of the target; a null image removes the cover.
    void setCover(CoverTarget target, const QImage &image);

    // The changed fields applied to a copy of the work (used by save()).
    Work applied(Work work) const;

signals:
    void workIdChanged();
    void loaded();
    void fieldsChanged();
    void modifiedChanged();
    void coverChanged();

private:
    // A cover change of one kind, waiting for save().
    struct PendingCover
    {
        bool changed = false;
        QImage image;          // null = no cover
        QString previewPath;   // the image in m_previewDir, for the view
    };
    PendingCover &pending(Format::Kind kind)
    {
        return kind == Format::Kind::Ebook ? m_ebookCover : m_audiobookCover;
    }
    const PendingCover &pending(Format::Kind kind) const
    {
        return kind == Format::Kind::Ebook ? m_ebookCover : m_audiobookCover;
    }
    QList<Format::Kind> kindsOf(CoverTarget target) const;
    QUrl coverUrl(Format::Kind kind) const;
    bool coverIsSquare(Format::Kind kind) const;
    QImage currentCover(Format::Kind kind) const;
    void clearCovers();
    // Writes the pending covers into the formats of work: the files written
    // and the old covers to delete once the work is stored.
    bool applyCovers(Work &work, QStringList *written, QStringList *replaced) const;

    // The editable values, as shown in the form.
    struct Fields
    {
        QString title;
        QString authors;
        QString series;
        int seriesIndex = 0;
        QString language;
        int publishedYear = 0;
        QStringList tags;
        int rating = 0;
        QString description;
        QString narrators;
        QString audiobookPublisher;
        QString audiobookIsbn;
        QString ebookPublisher;
        QString ebookIsbn;

        static Fields of(const Work &work);
        // Same values once trimmed and split as save() would store them.
        bool sameAs(const Fields &other) const;
    };

    void load(const Work &work);
    template <typename T>
    void setField(T Fields::*field, const T &value);
    void updateModified();

    QString m_workId;
    bool m_valid = false;
    bool m_modified = false;
    Work m_original;
    Fields m_originalFields;
    Fields m_fields;
    PendingCover m_ebookCover;
    PendingCover m_audiobookCover;
    std::unique_ptr<QTemporaryDir> m_previewDir;
    int m_previewNumber = 0;
};
