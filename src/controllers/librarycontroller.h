#pragma once

#include <QObject>
#include <QQmlEngine>

#include "workmodel.h"

class LibraryBackend;

// Entry point of the views to the library.
//
// In QML (import Calibly.Controllers):
//   LibraryController.works   WorkModel with all works
//   LibraryController.reload()  reads all works again from the backend
class LibraryController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(WorkModel *works READ works CONSTANT)

public:
    explicit LibraryController(LibraryBackend &backend, QQmlEngine *engine, QObject *parent = nullptr);
    ~LibraryController() override;

    // Used by the QML engine to obtain the singleton instance created in main().
    static LibraryController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    WorkModel *works() { return &m_works; }

    Q_INVOKABLE void reload();

private:
    LibraryBackend &m_backend;
    QQmlEngine *m_engine = nullptr;
    WorkModel m_works;

    static inline LibraryController *s_instance = nullptr;
};
