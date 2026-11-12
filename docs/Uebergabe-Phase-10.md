# Übergabe: Phase 10 (E-Book-Reader) – Stand 2026-11-12

Handoff note for the next Claude Code session (on any machine). Read this first, then `docs/UI-Development-Plan.md` (section "Phase 10") and the mockup `mockups/calibly-reader-hybrid.html`. Continue with "Missing", item 1.

## State

Phase 10 is about half done: the C++ part builds and is tested, the reader is not visible in the app yet (no QML, no hook-up). Last commit before this work: `0ac83b7 phase 10: planned reader functionality`.

### Done (C++, builds)
- **core:** `ReadingPosition` (`src/core/readingposition.*`, JSON: chapter + offset + label for EPUB/text, page for PDF); `Format::readingPosition`; SQLite schema 5 (`formats.reading_position`, migration in `SqliteBackend::open`); mock JSON key `readingPosition`.
- **new module `src/reader`** (lib `calibly_reader`, added to the root `CMakeLists.txt`):
  - `BookSource` – interface, path resolving (`resolvePath`, `fragmentOf`, `textLength`).
  - `EpubBook` – OPF spine (linear items only), contents from the EPUB 3 nav or the EPUB 2 NCX, font files, case-insensitive ZIP lookup, `decodeText()`.
  - `TextBook` – TXT/Markdown as one chapter; Markdown `#`/`##` headings become the contents (fragment `heading:<text>`).
  - `HtmlPrep` – inlines linked CSS; drops color, background, font-size, line-height, letter-/word-spacing, `font` and (unless "Original") `font-family`; converts em/rem/pt to px (QTextDocument only knows px); SVG cover → `<img>`; every `id` gets an `<a name>`.
  - `BookLayout` – one `QTextDocument` per chapter with page size and root-frame margins, LRU of 3; `pageCount`, `pageOf`, `offsetOfPage`, `offsetOfFragment`, `linkAt`, `paintPage`. Images are scaled to the text area and keep a single line height. The `TextStyle` struct is in `booklayout.h`.
- **controllers:**
  - `ReaderSettings` – QML singleton, text settings like Apple Books; 6 themes `original/ruhig/papier/kraeftig/sanft/fokus`; page colors as ids `weiss/sepia/grau/nacht`; stored in QSettings group `reader/`; a change by hand clears the theme.
  - `ReaderController` – QML singleton. `read(workId)` opens EPUB/PDF/TXT/MD in calibly, otherwise `LibraryController::openFormat`. Navigation, two-page spread when `twoPagesWanted` and the view is ≥ 880 px wide, background page counting, contents with pages, seek, links. PDF is drawn with C++ `QPdfDocument::render` (**not** `QtQuick.Pdf` – a deviation from the plan, still to be documented there); PDF zoom `fitPage/fitWidth/zoomIn/zoomOut`. Saves the position 1.5 s after moving, on close and on quit – only if the user moved (`m_moved`). Marks the format as opened; the last page sets progress 1; a finished book opens at its start; closes on `worksAboutToChange`.
  - `BookPage` – `QQuickPaintedItem` that draws through `ReaderController::paint`.
  - Created in `src/main.cpp` after `PlayerController`. The controllers link `Qt6::Quick`, `calibly_reader` and `Qt6::Pdf` (define `CALIBLY_HAS_QTPDF`).
- **tests:** `tests/tst_reader.cpp` 11/11 passed, `tests/tst_readercontroller.cpp` 11/11 passed. The other test programs were not rerun after these changes.

### Missing (in this order)
1. Run the whole `ctest --preset mingw-debug` once; add a SQLite migration test 4 → 5 in `tst_import.cpp`.
2. **QML** per the plan and the mockup:
   - Theme tokens: reader page colors for the four ids (incl. text and link colors), bar heights, contents width, text settings width, bar hide delay 3000 ms, fullscreen hint 2000 ms.
   - Control `ToggleSwitch`; components `ReaderBar`, `TableOfContents`, `TextSettings` (a panel; for PDFs it shows zoom instead).
   - View `ReaderView`: window mode = a view over the content area of `Main.qml`, contents column on the right, the player bar stays. Fullscreen button / F11 → `Window.FullScreen`, hides sidebar, PlayerBar and contents; the bars appear when the mouse moves over the page; a hint when entering; Esc leaves fullscreen, the next Esc closes the reader. Binds `ReaderController.viewSize`, `twoPagesWanted: fullscreen && ReaderSettings.twoPages`, the page/text/link colors from Theme by `ReaderSettings.pageColor`. Click zones: left/right third turns pages, the middle toggles the bars; links via `linkAt` / `followLink`. Keys ← → PgUp PgDn Space Home End, Ctrl + / Ctrl − for the font size.
   - In `Main.qml` the key handler must route keys to the reader while it is open instead of the player.
3. Hook "Lesen" to `ReaderController.read()`: `LibraryView.continueWith`, `ContinueView.continueWith`, `DetailDrawer` FormatCard `onOpenRequested` (close the drawer when the reader opens), context menu. Add "Mit Standardprogramm öffnen" (FormatCard for ebooks, `WorkMenu`, error state of the reader).
4. `ContinueModel` / `ContinueCard`: EPUB shows "<label> · 42 %" from `ReadingPosition.label`, PDF "Seite x von y" from the stored page.
5. Step 10.4: import TXT/Markdown only when picked or dropped as files, not when found while scanning folders (audiobook folders often contain .txt).
6. Component gallery entries, `update_translations` + English texts, the plan's "Implemented as" for phase 10 (incl. the deviations: `QPdfDocument` instead of `QtQuick.Pdf`, no links in PDFs), README.

## How the user works
- Asks for one phase of the plan at a time ("Fahre fort mit Phase N"), writes in German, commits each finished phase themselves as "phase N: …". Do not commit unless asked.
- A phase is done when it has: C++ logic with Qt Test tests, new components in all states in `ComponentGallery.qml`, only `Theme.*` values in QML, texts in `qsTr()` (German source, English translation), a property/signal comment at the top of each QML file, clean `all_qmllint`, an "Implemented as:" paragraph in the plan and a README update. Deviations from the plan are fine when explained there.
- Code and comments in English, UI texts in German.
- **No claude.ai Artifacts** unless explicitly asked. Mockups go to `mockups/` as plain HTML in the style of `calibly-cover-galerie-v2.html`. Ask before starting a headless browser (Edge) for PNGs.

## Build and verify (Windows, Qt as in the README)
- Build tools are not on PATH: `C:\Qt\Tools\CMake_64\bin` for `cmake` / `ctest` (preset `mingw-debug`); targets `all_qmllint`, `update_translations`.
- Running `calibly.exe` or a test exe directly needs `C:\Qt\6.12.0\mingw_64\bin` and `C:\Qt\Tools\mingw1510_64\bin` on PATH (not for the cmake build itself).
- Test executables print nothing in the agent shell: use `ctest`, or `tst_x.exe -o file.txt,txt` and read the file.
- Screenshots: `calibly.exe --mock ":/mock/mock.json" --size WxH --screenshot file.png` (also `--gallery`, `--language en`, `--library <folder>`, `--import <path>`). Start with `QT_FORCE_STDERR_LOGGING=1` and `2>&1`: QML runtime errors only show up there (a broken view still exits 0 with a blank area).
- The window is capped at the screen height: move gallery sections to the top temporarily for a screenshot. States that need clicks: add a short `Timer` to a copy, build, screenshot, restore.
- Clicks and drag & drop cannot be simulated: ask the user to try them.
- QML traps: an inline component of a file with `pragma ComponentBehavior: Bound` cannot be used from another file; `Row`/`Column` have no implicit size while invisible.
- Never run `tools/generate-mock-covers.ps1` just to probe it (it rewrites the committed covers).
- After changing texts: `update_translations`, then fill `i18n/calibly_en.ts` and the German plural forms in `calibly_de.ts`. Two German plurals (`%Ln Seiten` in MetadataForm, `%n Stern(e)`) were unfinished before phase 7.
