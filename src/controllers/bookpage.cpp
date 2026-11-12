#include "bookpage.h"

#include <QQuickWindow>

#include "readercontroller.h"

BookPage::BookPage(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    if (ReaderController *reader = ReaderController::instance()) {
        connect(reader, &ReaderController::positionChanged, this, [this] { update(); });
        connect(reader, &ReaderController::layoutChanged, this, [this] { update(); });
        connect(reader, &ReaderController::openChanged, this, [this] { update(); });
    }
}

void BookPage::paint(QPainter *painter)
{
    if (ReaderController *reader = ReaderController::instance())
        reader->paint(painter, size(), window() ? window()->effectiveDevicePixelRatio() : 1.0);
}
