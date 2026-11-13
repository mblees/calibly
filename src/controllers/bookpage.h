// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QQmlEngine>
#include <QQuickPaintedItem>

// The page(s) of the open book (EPUB, text or PDF) as ReaderController shows
// them. Size it with ReaderController.pageItemSize; it repaints whenever the
// page, the layout or the text settings change.
//
// In QML (import Calibly.Controllers):
//   BookPage { width: ReaderController.pageItemSize.width; height: … }
class BookPage : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit BookPage(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;
};
