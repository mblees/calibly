#pragma once

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

#include "workmodel.h" // complete type needed for the model property

// All details of one work, for the detail view. Follows changes of the
// model, e.g. after editing (phase 5) or reloading.
//
// In QML (import Calibly.Models):
//   WorkDetails {
//       model: LibraryController.works
//       workId: "moby-dick"
//   }
//
// formats is a list with one entry per format:
//   { kind: "ebook" | "audiobook", fileName, fileType ("EPUB", "M4B" …),
//     fileSize (bytes), pages, durationSeconds, progress (0–1),
//     available (the file exists) }
class WorkDetails : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(WorkModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString workId READ workId WRITE setWorkId NOTIFY workIdChanged)

    Q_PROPERTY(bool valid READ valid NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QStringList authors READ authors NOTIFY changed)
    Q_PROPERTY(QStringList narrators READ narrators NOTIFY changed)
    Q_PROPERTY(QString series READ series NOTIFY changed)
    Q_PROPERTY(int seriesIndex READ seriesIndex NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(QStringList tags READ tags NOTIFY changed)
    Q_PROPERTY(int rating READ rating NOTIFY changed)
    Q_PROPERTY(QString language READ language NOTIFY changed)
    Q_PROPERTY(int publishedYear READ publishedYear NOTIFY changed)
    Q_PROPERTY(QString publisher READ publisher NOTIFY changed)
    Q_PROPERTY(QDateTime added READ added NOTIFY changed)
    Q_PROPERTY(QUrl coverUrl READ coverUrl NOTIFY changed)
    Q_PROPERTY(bool coverIsSquare READ coverIsSquare NOTIFY changed)
    Q_PROPERTY(QVariantList formats READ formats NOTIFY changed)

public:
    explicit WorkDetails(QObject *parent = nullptr);

    WorkModel *model() const { return m_model; }
    void setModel(WorkModel *model);

    QString workId() const { return m_workId; }
    void setWorkId(const QString &workId);

    bool valid() const { return m_work != nullptr; }
    QString title() const;
    QStringList authors() const;
    QStringList narrators() const;
    QString series() const;
    int seriesIndex() const;
    QString description() const;
    QStringList tags() const;
    int rating() const;
    QString language() const;
    int publishedYear() const;
    QString publisher() const;
    QDateTime added() const;
    QUrl coverUrl() const;
    bool coverIsSquare() const;
    QVariantList formats() const;

signals:
    void modelChanged();
    void workIdChanged();
    void changed();

private:
    void refresh();

    QPointer<WorkModel> m_model;
    QString m_workId;
    const Work *m_work = nullptr;
    QList<QMetaObject::Connection> m_connections;
};
