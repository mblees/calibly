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
| Library for tags and chapters (e.g. TagLib) | 4 / 6 | Read metadata and chapters from audio files. Qt cannot do this sufficiently. |
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
| `coverUrl`, `coverIsSquare` | url / bool | Controls how square covers are displayed |
| `hasEbook`, `hasAudiobook` | bool | Determines the format icons |
| `readProgress`, `listenProgress` | real 0–1 | Progress lines |
| `state` | enum | `Normal`, `New`, `Finished`, `FileMissing` |
| `lastOpened` | datetime | Sorting and "Continue" |

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
    property int  state: WorkState.Normal
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
- **Language:** all texts are wrapped in `qsTr()` from the start. Identifiers in code are English.

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
| `ProgressLine` | Controls | A, B, C | 1 |
| `Badge` / `Label` | Controls | F | 1 |
| `CoverArt` (2:3 and square, series stack) | Components | A, B, C, D, E | 1 |
| `CoverTile` (icons, labels, progress, hover, selection) | Components | A, D | 1 / 8 / 9 |
| `SideDrawer`, `Tabs` | Controls | B | 3 |
| `FormatCard` | Components | B | 3 |
| `FormField` | Controls | E | 5 |
| `MetadataForm` | Components | E | 5 |
| `MiniPlayer`, `PlayerControls`, `ChapterList` | Components | A, C | 6 |
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

### Phase 5 – Edit metadata
- `MetadataEditor` dialog with `MetadataForm`. Shared fields (🔗) and format-specific fields.
- Tabs General, Audiobook, Ebook. Navigation to the previous and next title.
- No online suggestions yet.
- **Done when:** changes are saved and immediately visible in the gallery and the detail view.

### Phase 6 – Audiobook player
- `PlayerController` (C++) based on Qt Multimedia. It stores the position per audiobook.
- `MiniPlayer` at the bottom of the library. The fullscreen view `PlayerView` shows the progress within the **chapter**, the overall progress in small print and the chapter list.
- Playback speed and sleep timer. Space for play/pause, ← / → skip 30 seconds.
- Chapters come from the M4B file or from the files of an MP3 folder.
- **Done when:** an audiobook resumes at the same position after a restart.

### Phase 7 – Drag & drop
- `DropArea` on the gallery and on every cover. A hint shows what will happen on release.
- `ImportController` detects audiobook folders and merges MP3 files into one title.
- `ImportQueue` at the bottom right with a status per entry (done, in progress, needs review).
- **Done when:** both cases from view D work, i.e. new work and adding a format.

### Phase 8 – "Continue" and cover states
- "Continue" row with `ContinueCard`, sorted by last opened.
- States on the cover: New, Finished, File missing.
- **Done when:** every state from view F can appear in the real gallery.

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
| Copy files into a library folder or leave them in place? | Copy like calibre, leaving in place as an option | Phase 4 |
| Library for tags and chapters | Evaluate TagLib | Phase 4 |
| UI language: German only or also English? | German first, `qsTr()` from the start | Phase 0 |
| Rendering for the ebook reader | separate plan | after phase 10 |
