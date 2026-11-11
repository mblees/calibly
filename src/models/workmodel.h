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
        CoverUrlRole,    // QUrl, empty = placeholder
        CoverIsSquareRole,
        HasEbookRole,
        HasAudiobookRole,
        ReadProgressRole,   // 0.0 – 1.0
        ListenProgressRole, // 0.0 – 1.0
        LastOpenedRole,
    };
    Q_ENUM(Role)

    explicit WorkModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_works.size()); }
    void setWorks(QList<Work> works);

signals:
    void countChanged();

private:
    QList<Work> m_works;
};
