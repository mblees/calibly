# calibly – UI Development Plan

Status: 2026-11-11 · Based on: [Mockup "Cover Gallery, Version 2"](../mockups/calibly-cover-galerie-v2.png) · Implementation with Qt Quick

---

## 1. Concept

### Core idea
calibly is a **personal tool**, not a commercial product. It manages ebooks and audiobooks in **one** library and presents them as a shelf of covers. Only what is needed in everyday use gets built – a few features that work well rather than many.

### Key terms
| Term | Meaning |
|---|---|
| **Work** | A title, independent of its format (e.g. "Moby Dick"). The UI always operates on works. |
| **Format** | The concrete file(s) of a work: **ebook** (EPUB, PDF, …) or **audiobook** (M4B, MP3 folder, …). A work has one or more formats. |
| **Progress** | Tracked separately per format: reading (page / %) and listening (time / %). No automatic syncing. |
| **Series** | Works with a series name and volume number. Shown as a stack in the gallery. |

### Color semantics
| Color | Meaning |
|---|---|
| Blue `#6ea8ff` | Reading / ebook |
| Orange `#ff8a3d` | Listening / audiobook, also the accent color (selection, primary action) |
| Green `#5fcf80` | Finished |
| Background `#16171b`, card `#212228` | Dark base theme |

### Views
| | View | Essentials |
|---|---|---|
| A | **Library** | Sidebar, search, format selection (All / Ebooks / Audiobooks / Both), sorting, "Continue" row, cover grid, player bar at the bottom. |
| B | **Detail** | Slides in from the right when a cover is clicked. One card per format with progress and "Continue reading" / "Continue listening". Tabs: Overview, Chapters, Notes, Files. |
| C | **Player (fullscreen)** | Large cover, progress bar relative to the **current chapter**, overall progress shown small below it. Playback speed, sleep timer, chapter list. |
| D | **Drag & drop** | Drop onto empty space = new work. Drop onto a cover = add the format to that work. Import progress at the bottom right. |
| E | **Edit metadata** | One form for all formats. Shared fields (🔗) apply to all formats, format-specific ones (narrator, ISBN, …) are separate. Online suggestions with before/after. |
| F | **Cover states** | Binding list: ebook only, audiobook only, both, read x %, read + listened, finished, series, new, selected, file missing. |

### Interaction principles
1. **Clicking a cover** opens the detail view. **Hovering over a cover** shows "Read" / "Listen" to continue directly.
2. Nothing is reachable **only** by hover. Every hover action is also available in the detail view or the "⋯" menu (important for touch devices later).
3. A **cover without text** shows what you own and where you are: format icons at the top right, progress lines at the bottom, state label at the top left.
4. **Square audiobook covers** are centered in the uniform 2:3 grid, with the remaining space filled by a blurred version of the cover.
5. Files enter the library **via drag & drop** or via "Add".
6. The selection **Ebooks** or **Audiobooks** shows every work that has this format, presented as that format only: a work with both formats then shows only the cover, badge and progress of the selected format. Each format has its own cover; with no format selected the ebook cover represents the work.

### Deliberately excluded
Devices / "Send to device", collections, smart lists, additional filters (status, language), history, the suggestion to sync positions, bookmarks, queue and "Follow along in text" in the player.
The architecture should not prevent these later. But they will not be built.

---

## 2. Technical guidelines

### Dependencies
**Start: only Qt 6 with Qt Quick** (current LTS version, e.g. 6.8), C++17 and CMake.
Everything the first phases need is already included:

| Qt Quick module | Used for |
|---|---|
| `QtQuick`, `QtQuick.Layouts` | Basics, grid, layouts, `DropArea` |
| `QtQuick.Controls` | Base for buttons, fields, tabs, drawer |
| `QtQuick.Shapes` | Icons as SVG paths (`PathSvg`), so no separate SVG module is needed |
| `QtQuick.Effects` | Blur for square covers and the player background (`MultiEffect`) |
| `QtQuick.Dialogs` | File dialog for "Add" |

Further dependencies are only added in the phase that needs them:

| Dependency | From phase | Purpose |
|---|---|---|
| Qt SQL (SQLite, part of qtbase) | 4 | Store the library |
| TagLib 2.3.2 (built from source via CMake FetchContent, LGPL/MPL) | 4 | Metadata, length and cover of audio files; also has MP4 chapters for phase 6 |
| Qt-internal ZIP reader (`QZipReader`, Qt Core private) | 4 | Read EPUB files without an extra ZIP library |
| Qt PDF (optional, Qt Maintenance Tool → Extensions) | 4 | Cover (first page), title, author and page count of PDFs; without it PDFs get placeholders |
| Qt Multimedia | 6 | Audio playback |
| Qt Network | 10 | Online metadata |
| Ebook rendering | separate plan | Integrated reader |

> **Assumption: C++ rather than Python.** Qt Quick with C++ has the best support for porting to Linux, Android and iOS later. PySide6 would work for Windows and Linux, but is weaker on mobile.

### Layers
```
┌─────────────────────────────────────────────────────────────┐
│ Views        Screens: LibraryView, DetailDrawer, PlayerView  │
│ Components   Domain building blocks: CoverTile, MiniPlayer … │
│ Controls     Neutral building blocks: Chip, AppButton, …     │
│ Theme        Colors, spacing, fonts, icons                   │
├──────────── Boundary: properties, signals, models only ──────┤
│ C++ models & controllers   WorkModel, PlayerController …     │
│ C++ backend                mock first, later SQLite + files  │
└─────────────────────────────────────────────────────────────┘
```

**Rules**
1. Each layer only uses the layers below it.
2. **Controls** know nothing about books. They could be used in any other app.
3. **Components** receive data only through properties (`required property`) and report actions only through signals. They never access controllers or models themselves.
4. Only **views** connect components to models and controllers.
5. Colors, spacing and font sizes come only from the **theme**. Components contain no hard-coded color values.
6. QML contains **no business logic**. Filtering, sorting and import decisions are handled in C++.
7. The backend sits behind an interface (`LibraryBackend`). First there is a `MockBackend`, later a `SqliteBackend`. The UI stays unchanged.

### Folder structure
```
calibly/
├─ CMakeLists.txt
├─ src/
│  ├─ main.cpp
│  ├─ core/          Work, Format, Chapter (plain data classes)
│  ├─ backend/       LibraryBackend (interface), MockBackend, later SqliteBackend
│  ├─ models/        WorkModel, WorkFilterModel, ChapterModel, ImportQueueModel
│  └─ controllers/   LibraryController, PlayerController, ImportController
├─ qml/
│  ├─ Theme/         Theme.qml (singleton), Icons.qml (path data)
│  ├─ Controls/      neutral building blocks
│  ├─ Components/    domain building blocks
│  ├─ Views/         screens + ComponentGallery.qml
│  └─ Main.qml       window, sidebar, navigation
├─ resources/        mock covers, mock data (JSON)
├─ tests/            Qt Quick Test, C++ tests for the models
├─ docs/
└─ mockups/
```
Each QML folder is its own module (`qt_add_qml_module`): `Calibly.Theme`, `Calibly.Controls`, `Calibly.Components`, `Calibly.Views`. This gives clear imports and modules that can be tested separately. In addition, `qmllint` checks the code and QML is compiled at build time.

### Data model from the UI's perspective
`WorkModel` (`QAbstractListModel`) provides the following roles per work:

| Role | Type | Note |
|---|---|---|
| `workId`, `title`, `authors`, `narrators` | string / list | |
| `series`, `seriesIndex` | string / int | |
| `coverUrl`, `coverIsSquare` | url / bool | Cover of the shown format (ebook, else audiobook; in the Ebooks/Audiobooks selection that format). Controls how square covers are displayed |
| `hasEbook`, `hasAudiobook` | bool | Determines the format icons |
| `readProgress`, `listenProgress` | real 0–1 | Progress lines |
| `workState` | enum `WorkState` | `Normal`, `New`, `Finished`, `FileMissing` (not `state`: every QML item already has a property of that name) |
| `lastOpened` | datetime | Sorting; "Continue" uses the date of the format (`ContinueModel`) |

`WorkFilterModel` (`QSortFilterProxyModel`) has the properties `formatFilter` (All / Ebook / Audiobook / Both), `searchText` and `sortOrder`.

### Example: component API
```qml
// qml/Components/CoverTile.qml – presentation only, no logic
Item {
    required property string title
    required property string author
    required property url    coverUrl
    required property bool   coverIsSquare
    required property bool   hasEbook
    required property bool   hasAudiobook
    property real readProgress: 0
    property real listenProgress: 0
    property int  workState: WorkState.Normal
    property bool selected: false

    signal clicked()
    signal readRequested()
    signal listenRequested()
    signal selectToggled()
    // Colors and sizes only via Theme.*
}
```

### Extensibility
- **New view:** a new file in `Views/` and an entry in the navigation model. The sidebar is built from this model.
- **New file formats** (e.g. OPUS, AZW3): only added in the backend. The UI only distinguishes the categories ebook and audiobook.
- **New cover state:** a new enum value and its presentation, both in exactly one place in `CoverTile`.
- **Light theme:** components only use theme values, so a second set of values is enough.
- **Mobile devices:** no fixed window sizes, column count derived from the available width, hover actions also available via long press.
- **Language:** all texts are wrapped in `qsTr()` from the start. Identifiers in code are English. See "Translations" below.

### Translations
Uses only Qt's own tools (Qt Linguist, part of the Qt installation):
- Texts in the source code are **German** (source language). Each further language has its own file `i18n/calibly_<code>.ts`, currently English.
- `update_translations` (lupdate) collects all `qsTr()` texts into the `.ts` files. They are translated with Qt Linguist (`linguist.exe`) or directly in the XML.
- On every build, lrelease compiles them into `.qm` files, which are embedded into the app under `:/i18n`. Qt's own texts (e.g. standard dialogs) are merged in.
- The current language is `Qt.uiLanguage`. `LanguageManager` (module `Calibly.Controllers`) reacts to changes: it installs the translation, calls `QQmlEngine::retranslate()` and stores the choice. Switching therefore works at runtime, without a restart.
- Start language: `--language <code>` > saved choice > system language > German.
- **New language:** add the code to `I18N_TRANSLATED_LANGUAGES` in `CMakeLists.txt`, run `update_translations`, translate the new `.ts` file. It then appears automatically in `LanguageManager.languages`.
- Rules for QML: texts that must follow a language change have to be bindings (`text: qsTr("…")` or `Qt.binding(…)`), never one-off assignments. Numbers and dates are formatted with `Qt.locale(Qt.uiLanguage)`.

### Component gallery
View F from the mockup becomes a real developer view `ComponentGallery.qml` (started with `--gallery`). It shows every component in all its states with mock data. **New components are created there first** and only then added to a view. The gallery also serves as a quick visual test.

### Component list
| Component | Layer | Used in | Phase |
|---|---|---|---|
| `Theme`, `Icon` | Theme | everywhere | 0 |
| `AppButton`, `IconButton` | Controls | everywhere | 0 |
| `SidebarNav` | Components | A | 0 |
| `Chip`, `ChipGroup` | Controls | A | 2 |
| `SearchField` | Controls | A | 2 |
| `SelectMenu` (sort menu), `AppSlider` (cover size) | Controls | A | 2 |
| `ProgressLine` | Controls | A, B, C | 1 |
| `Badge` / `Label` | Controls | F | 1 |
| `CoverArt` (2:3 and square, series stack) | Components | A, B, C, D, E | 1 |
| `CoverTile` (icons, labels, progress, hover, selection) | Components | A, D | 1 / 8 / 9 |
| `SideDrawer`, `Tabs`, `RatingStars` | Controls | B | 3 |
| `FormatCard` | Components | B | 3 |
| `LinkButton` ("Als fertig markieren") | Controls | B | 8 |
| `FormField`, `InputField`, `TextAreaField`, `TagField` | Controls | E | 5 |
| `MetadataForm` | Components | E | 5 |
| `SeekBar` | Controls | A, C | 6 |
| `MiniPlayer`, `PlayerControls`, `PlaybackOptions`, `ChapterList` | Components | A, B, C | 6 |
| `DropHint`, `ImportQueue` | Components | D | 7 |
| `ContinueCard` | Components | A | 8 |
| `BulkActionBar` | Components | A | 9 |
| `SuggestionList` | Components | E | 10 |

---

## 3. Step-by-step build-up

Every phase ends with a **runnable app**. A phase only starts once the previous one is usable.

| Phase | Result | New dependency |
|---|---|---|
| 0 | Empty window with sidebar and theme | Qt Quick |
| 1 | Cover grid with mock data | – |
| 2 | Search, format selection, sorting | – |
| 3 | Detail view (display only) | – |
| 4 | Real library: adding and storing | Qt SQL, tag library |
| 5 | Edit metadata | – |
| 6 | Audiobook player (bar and fullscreen) | Qt Multimedia |
| 7 | Drag & drop with import queue | – |
| 8 | "Continue" and cover states | – |
| 9 | Multi-selection, merging works, series | – |
| 10 | Online metadata suggestions | Qt Network |

### Phase 0 – Skeleton
- CMake project, folder and module structure as described above.
- `Theme` singleton with colors, spacing and font sizes. `Icon` based on `QtQuick.Shapes`.
- `Main.qml` with sidebar (Library, Continue, Series, Authors, Narrators) and a placeholder for the content.
- Empty `ComponentGallery`.
- **Done when:** the app starts, navigation switches between placeholders and `qmllint` runs without warnings.

### Phase 1 – Static gallery
- `MockBackend` reads sample works from `resources/mock.json`. The `WorkModel` is built from them.
- `GridView` with `CoverTile`: cover, title, author, format icons, progress lines.
- `CoverArt` displays square audiobook covers and fills the remaining space with a blur.
- The number of columns follows the window width.
- **Done when:** the gallery looks like the mockup, scrolls smoothly and all variants are visible in the `ComponentGallery`.

### Phase 2 – Search, format selection, sorting
- `WorkFilterModel` in C++. Plus `ChipGroup` (All / Ebooks / Audiobooks / Both), `SearchField` and a sort menu.
- Optionally a slider for the cover size.
- **Done when:** selection, search and sorting can be combined and the display updates immediately.

### Phase 3 – Detail view
- `SideDrawer` with cover, title, author, narrator, series, rating and tags.
- One `FormatCard` per format. "Continue reading" and "Continue listening" initially open the file with the system's default application (`Qt.openUrlExternally`).
- Only the "Overview" tab at first. Chapters, Notes and Files follow in later phases.
- **Done when:** clicking a cover opens the detail view and it closes with Esc or ✕.

### Phase 4 – Real library
- `SqliteBackend` replaces the `MockBackend`. The UI stays unchanged.
- "Add" via the file dialog. Title, author, cover and duration are read from the files.
- The decisions on the tag library and on where files are stored are made here (see open decisions).
- **Done when:** imported titles are still there after a restart.

**Implemented as:** `SqliteBackend` keeps `calibly.db` inside the library folder with paths relative to it, so the folder can be moved as a whole. Import (`src/import`) runs in a background thread: read metadata (EPUB, TagLib, Qt PDF, file name such as `DCC 01 - Title - Author` as fallback), group files into works (same base name or same title and author; ebook metadata wins), copy into the library folder, save covers next to the files. Files already in the library (same format and size) are skipped.

### Phase 5 – Edit metadata
- `MetadataEditor` dialog with `MetadataForm`. Shared fields (🔗) and format-specific fields.
- Tabs General, Audiobook, Ebook. Navigation to the previous and next title.
- No online suggestions yet.
- **Done when:** changes are saved and immediately visible in the gallery and the detail view.

**Implemented as:** `WorkEditor` (C++, `Calibly.Controllers`) holds an editable copy of one work, knows whether it was changed and saves through `LibraryController::saveWork()`, which updates the backend and the `WorkModel` – so the gallery and the detail view follow at once. The dialog `MetadataEditor` (opened with ✎ in the detail view) shows `MetadataForm` with the tabs General, Audiobook and Ebook (only the formats the work has). Shared fields: title, authors, series and volume, language, year, tags, rating, description. Format fields: narrators, publisher and ISBN of the audiobook; publisher and ISBN of the ebook. Publisher and ISBN are therefore stored per format (database schema 2; older libraries are updated on open), and the ISBN is read from EPUB identifiers on import. ‹ / › (Ctrl+PgUp / Ctrl+PgDown) step through the titles of the current gallery selection and keep the changes made so far, like calibre; Ctrl+S saves and closes; Esc, ✕ and "Cancel" drop unsaved changes. Changing the cover is not part of this phase.

### Phase 6 – Audiobook player
- `PlayerController` (C++) based on Qt Multimedia. It stores the position per audiobook.
- `MiniPlayer` at the bottom of the library. The fullscreen view `PlayerView` shows the progress within the **chapter**, the overall progress in small print and the chapter list.
- Playback speed and sleep timer. Space for play/pause, ← / → skip 30 seconds.
- Chapters come from the M4B file or from the files of an MP3 folder.
- **Done when:** an audiobook resumes at the same position after a restart.

**Implemented as:** `AudioTimeline` (core) puts the files and chapters of an audiobook on one time axis, so an MP3 folder plays like one file. Chapters are read on import (M4B/M4A: QuickTime or Nero chapters via TagLib, MP3: ID3v2 CHAP frames); the files of an MP3 folder become one audiobook format with one chapter per file, ordered by folder, track number and file name. Chapters and the listening position are stored per format (database schema 3). `PlayerController` (Qt Multimedia) plays, follows the chapter, saves the position on pause, on chapter changes, every 15 seconds and on quit, and loads the last audiobook paused on the next start. Speed (0.75–2.5×, with pitch correction) and sleep timer (minutes or end of chapter, fades out) are part of the controller. The player bar (`PlayerBar` with `MiniPlayer`) sits below the library; it opens `PlayerView` in fullscreen. "Continue listening" in the detail view plays in calibly instead of the default application, and the detail view got the "Chapters" tab. Space plays / pauses, ← / → skip 30 seconds when no control uses these keys. Audiobooks imported in phase 4 from an MP3 folder stay several formats; joining them is part of phase 9 (merging).

### Phase 7 – Drag & drop
- `DropArea` on the gallery and on every cover. A hint shows what will happen on release.
- `ImportController` detects audiobook folders and merges MP3 files into one title.
- `ImportQueue` at the bottom right with a status per entry (done, in progress, needs review).
- **Done when:** both cases from view D work, i.e. new work and adding a format.

**Implemented as:** `ImportController` (C++, `Calibly.Controllers`) took over importing from `LibraryController`. Every drop or "Add" is a job; jobs run one after the other in the import thread, so files can be dropped while an import runs. Each work is saved as soon as its files are copied and appears in the gallery right away (`WorkModel::updateWork()` now adds new works). `ImportQueueModel` (`Calibly.Models`) holds the queue: one row per job while its files are read, then one row per work with the states waiting, reading, copying, done, needs review, failed and skipped. Audiobook folders: `groupAudioFolders()` gives the audio parts of one folder (also split into "CD 1", "CD 2" … subfolders) one title – from the album tag, otherwise from the folder name ("Dracula - Bram Stoker") – unless their album tags differ; M4B files stay single audiobooks, and file base names no longer join audio parts ("01.mp3" exists in many folders). A new work whose title is only the file name and that has no author "needs review"; "Prüfen" opens the metadata editor. Instead of one `DropArea` per cover, `LibraryView` has one `DropArea` that finds the cover under the pointer (the gap between columns counts as free space) and scrolls the grid near its edges; dropping on a cover passes its work id, and `planImport()` then adds every file that is not a duplicate to that work. `DropHint` shows the two cases at the top of the grid, `CoverTile.dropLabel` highlights the target cover ("Als Hörbuch zu „Dracula“ hinzufügen"), `ImportQueue` sits at the bottom right and hides itself after a while when nothing needs attention. New control: `Spinner`.

### Phase 8 – "Continue" and cover states
- "Continue" row with `ContinueCard`, sorted by last opened.
- States on the cover: New, Finished, File missing.
- **Done when:** every state from view F can appear in the real gallery.

**Implemented as:** `Work::state()` (core) decides the state of a cover, in this order: **file missing** (a file of a format was not found), **finished** (a format has progress 1), **new** (never opened since it was added, as in the mockup – there is no time limit). The models report it as the role `workState` with the QML enum `WorkState`; with the format selection "Ebooks" / "Audiobooks" it is the state of that format only. `CoverTile` shows the label at the top left ("NEU", "✓ FERTIG", "DATEI FEHLT"), draws the progress line of a finished format in green and the cover of a missing file in grey. Whether the files exist is checked when the library is loaded (`Format::fileMissing`, not stored; the sample data marks one title with `"missing": true`) and again when a file is opened or played. Because ebooks still open in another application, calibly cannot know when one is finished: the detail view got "Als fertig markieren" / "Als nicht fertig markieren" on every `FormatCard` (`LibraryController::setFinished()`; an audiobook returns to its listening position). The player now only stores a position it moved itself, so loading the last audiobook at start no longer resets a finished one.
"Continue": formats have their own `lastOpened` (database schema 4; older libraries take it over from the work for the formats with progress). `ContinueModel` (`Calibly.Models`) lists per work the unfinished format that was opened last, most recent first, following the format selection; the row hides while searching and shows at most 12 titles. `ContinueRow` (a view part) is the header of the cover grid together with the "Bibliothek" heading, so it scrolls away with the covers. A `ContinueCard` shows chapter and remaining time or page, the progress, "zuletzt: gestern, 22:40" and a round button that continues directly; a click on the card opens the detail view. An ebook that was opened stays in the row (without known progress) until it is marked as finished.
Hover actions (interaction principle 1): hovering a cover shows "Lesen" / "Hören" for the formats it has; they do the same as the buttons in the detail view.
The sidebar entry "Weitermachen" opens `ContinueView`: all titles that were started (opened, or with progress) and are not finished, as a grid of `ContinueCard`s. It uses a `ContinueModel` without limit and format selection, so titles and order are the same as in the row of the library; a work whose started formats are all finished is not listed. The entry shows their number. A click on a card opens the detail view, the round button continues directly.

### Phase 9 – Multi-selection and series
- Selection via the circle on the cover, Ctrl+click or Ctrl+A. Plus a `BulkActionBar` with Metadata, "Merge into one work" and Remove.
- Series stacks in the gallery. Clicking one opens the series.
- **Done when:** a standalone ebook and a standalone audiobook can be merged into one work.

### Phase 10 – Online suggestions
- `SuggestionList` in the metadata dialog with before/after and per-field adoption.
- Each source is its own plugin behind an interface (`MetadataSource`).

### Afterwards (separate plans)
- **Integrated ebook reader.** This is the largest single building block and requires a decision on rendering. Until then, "Read" opens the system's default application.
- Light theme, Linux build, adaptation for Android and iOS.

---

## 4. Way of working
- Small steps. The app starts after every step.
- New components are first created in the `ComponentGallery` with mock data. Only then are they added to a view.
- **A component is done** when it only uses theme values, all texts are wrapped in `qsTr()`, it is shown in all states in the gallery and its properties and signals are documented at the top of the file.
- `qmllint` runs as part of the build. C++ models get tests (Qt Test) as soon as they contain logic (from phase 2).

## 5. Open decisions
| Question | Recommendation | Latest by |
|---|---|---|
| C++ or Python (PySide6)? | C++, because of mobile devices later | Phase 0 |
| ~~Copy files into a library folder or leave them in place?~~ | **Decided:** copied into the library folder (`Documents\calibly-Bibliothek`, structure `<Author>/<Title>/`), like calibre | done |
| ~~Library for tags and chapters~~ | **Decided:** TagLib for audio, Qt's ZIP reader for EPUB, Qt PDF for PDFs | done |
| ~~UI language: German only or also English?~~ | **Decided:** German source texts, English translation, switchable at runtime (see "Translations") | done |
| Rendering for the ebook reader | separate plan | after phase 10 |
