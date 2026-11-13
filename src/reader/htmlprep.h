// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

#include <functional>

// Prepares the XHTML of an ebook chapter for QTextDocument (option A of the
// reader, see docs/UI-Development-Plan.md, phase 10):
//   * linked style sheets are inlined (QTextDocument does not load them),
//   * the book's CSS loses what the reader's text settings decide: colors,
//     backgrounds, font sizes, line height, letter and word spacing and,
//     unless the book's own fonts are wanted, font families. Alignment,
//     margins, indents, bold and italic stay; their em, rem and pt lengths
//     become pixels (emPixels per em), because QTextDocument only knows px,
//   * an SVG that only wraps an image (typical for cover pages) becomes <img>,
//   * every element with an id gets an <a name> so links to it can be found.
namespace HtmlPrep {

// stylesheet(href) returns the CSS of a linked style sheet, "" if unknown.
QString prepare(const QString &html, bool keepFonts, int emPixels,
                const std::function<QString(const QString &href)> &stylesheet);

// A style sheet without the properties named above; emPixels 0 keeps lengths.
QString cleanCss(const QString &css, bool keepFonts, int emPixels = 0);

// The declarations of one rule or style attribute ("color: red; margin: 0").
QString cleanDeclarations(const QString &declarations, bool keepFonts, int emPixels = 0);

} // namespace HtmlPrep
