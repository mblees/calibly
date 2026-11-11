#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>

#include "work.h"

// List of all works for the UI. One row per work; the roles are what QML
// delegates use as required properties (see docs/UI-Development-Plan.md).
class WorkModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by LibraryController.works")

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        WorkIdRole = Qt::UserRole + 1,
        TitleRole,
        AuthorsRole,     // QStringList
        AuthorTextRole,  // authors joined for display, e.g. "Jacob Grimm, Wilhelm Grimm"
        NarratorsRole,   // QStringList
        SeriesRole,
        SeriesIndexRole,
        CoverUrlRole,    // QUrl of the primary format (ebook, else audiobook), empty = placeholder
        CoverIsSquareRole,
        HasEbookRole,
        HasAudiobookRole,
        ReadProgressRole,   // 0.0 – 1.0
        ListenProgressRole, // 0.0 – 1.0
        LastOpenedRole,
        AuthorSortRole,     // sort key of the first author, e.g. "Goethe"
        SearchTextRole,     // folded title, authors, narrators, series and tags (see TextUtils)
        EbookCoverUrlRole,  // cover of the ebook, see Work::coverPath()
        EbookCoverIsSquareRole,
        AudiobookCoverUrlRole,
        AudiobookCoverIsSquareRole,
    };
    Q_ENUM(Role)

    explicit WorkModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_works.size()); }
    void setWorks(QList<Work> works);

    // Replaces the work with the same id (e.g. after opening it).
    void updateWork(const Work &work);

    // Row of the work with this id, -1 if there is none.
    int rowOf(const QString &workId) const;
    // Work in this row, nullptr for an invalid row.
    const Work *workAt(int row) const;

signals:
    void countChanged();

private:
    QList<Work> m_works;
    QStringList m_searchTexts; // one per work, precomputed in setWorks()
};
