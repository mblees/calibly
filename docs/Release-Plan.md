# calibly – Release Plan

From the finished feature phases (1–10 of the [UI Development Plan](UI-Development-Plan.md)) to a public **0.1 Beta** and then **1.0**.

## 1. Decisions (2026-11-12)

| Question | Decision |
|---|---|
| Audience | **Public**: anyone can download calibly. |
| Source code | **Closed**, all rights stay with the author. The repository `mblees/calibly` stays private. |
| Where people download | ~~A separate public repository for releases (`mblees/calibly-releases`).~~ **Changed 2026-11-12:** the releases are published in the working repository `mblees/calibly` (private for now; the update check needs it to be public). |
| Package | **Installer** (`calibly-<version>-setup.exe`). |
| Updates | **Update hint in the app**: at start calibly asks the public releases repository for the latest version and shows a hint with a link. Can be switched off. No automatic download. |
| Releases | **0.1 Beta** as a GitHub pre-release first, then **1.0** after the beta feedback. |
| New features before 1.0 | **Settings page** (in the beta) and the views **"Autoren" and "Sprecher"** (by 1.0). Everything else in "Afterwards" of the UI plan stays after 1.0. |

The concept of the UI plan calls calibly a personal tool; that stays true for the scope ("only what is needed"), but the release work below treats it as software that strangers install.

## 2. What is missing today

Found while looking through the code (state of commit `26060fb`):

**Packaging**
- No deployment: nothing collects the Qt DLLs, QML modules and plugins next to `calibly.exe` (no `install()` rules, no `windeployqt`).
- No installer, no app icon (the window and the `.exe` show the default icon), no Windows version information in the `.exe`.
- Project version `0.1.0` is set in CMake and passed as `CALIBLY_VERSION`, but nowhere shown.
- The sample library (`resources/mock`, about 30 covers) is built into every `calibly.exe`.

**Legal**
- No license text for calibly itself.
- No notices for the third-party parts: Qt (LGPL 3), TagLib (LGPL 2.1 / MPL 1.1, linked **statically**), FFmpeg inside Qt Multimedia (LGPL), PDFium inside Qt PDF (BSD), Feather Icons (MIT).

**Robustness**
- If the library cannot be opened, calibly **silently shows the sample data** (`main.cpp`). A user would think the library is gone, and changes go nowhere.
- Two running copies of calibly would write to the same `calibly.db`.
- No backup of `calibly.db` before a schema migration (schema 5 today; every phase added one).
- No log file: a user cannot send anything useful with a bug report.
- The EPUB reading uses Qt's private `QZipReader`; it can change with any Qt version.

**User interface**
- The language can only be switched in the component gallery; the library folder only with `--library`.
- "Autoren" and "Sprecher" in the sidebar are placeholders.
- No "Über calibly" (version, licenses).

**Tested only by the developer**: the reader (phase 10) has hardly been used in daily life yet; there was no test on a Windows without Qt installed.

## 3. Steps

Each step ends with a runnable app, as in the UI plan. Steps 1–4 lead to the beta, 5–6 to 1.0.

### Step 1 – Release build and installer
1. **Version and icon.** One version in `CMakeLists.txt` (`0.1.0`, the beta suffix in the installer and release name), shown in "Über calibly" later. App icon as `.ico` (16–256 px) from the logo, a Windows resource file (`.rc`) with icon and version information (product name, version, copyright line with the author's name), and the window icon.
2. **Sample data only for development.** A CMake option `CALIBLY_DEV_TOOLS` (on in `mingw-debug`, off in `mingw-release`) decides whether `--mock`, `--gallery`, `--screenshot` and the sample covers are built in.
3. **Deployment.** `install()` rules and `qt_generate_deploy_qml_app_script()` (Qt's CMake wrapper around `windeployqt`): `cmake --install` writes a complete folder with `calibly.exe`, the Qt and MinGW DLLs, QML modules, the multimedia (FFmpeg) and PDF plugins and the SQLite driver. Leave out what is not used (e.g. Qt Quick Controls styles other than Basic, unused image formats).
4. **Installer** with Inno Setup (free, one script file in `installer/calibly.iss`):
   - installs **per user** to `%LOCALAPPDATA%\Programs\calibly` (no admin rights, updates without UAC prompt);
   - start-menu entry, optional desktop icon, uninstaller in "Apps";
   - shows the license (step 2) before installing; German and English pages;
   - **uninstalling never touches the library** (`Documents\calibly-Bibliothek`) or the settings;
   - installing a newer version over an older one keeps everything.
   - A build target or script `tools/make-release.ps1`: release build → tests → `cmake --install` → Inno Setup → `calibly-0.1.0-beta-setup.exe`.
5. **Test on a clean Windows**: Windows Sandbox (part of Windows 11 Pro) has no Qt. Install, import an EPUB, a PDF, an M4B and an MP3 folder, read, play, uninstall.
- **Done when:** the setup file installs calibly on a clean Windows without admin rights, every feature works there, and uninstalling leaves the library in place.

**Implemented as:** The release information is at the top of `CMakeLists.txt`: `PROJECT_VERSION` 0.1.0, `CALIBLY_VERSION_SUFFIX` "beta" (the app, the `.exe` properties and the installer show `0.1.0-beta`), publisher, copyright line and the releases page. The icon (book spines on a shelf whose heights form an equalizer, blue and orange in turn, on the dark background; idea 4 of `mockups/calibly-logo-ideen.html`, it replaced the first icon of a book wearing headphones) is drawn by `tools/generate-icon.ps1` into `resources/icon/calibly.ico` (16–256 px) and `calibly.png`; `resources/windows/calibly.rc.in` makes it the icon of `calibly.exe` (`IDI_ICON1`, which Qt also uses for the window) and adds the version information. The CMake option `CALIBLY_DEV_TOOLS` (default on, off in `mingw-release`) builds in `--mock`, `--gallery`, `--screenshot`, `--size`, the target `run-gallery` and the sample library; without it these options do not exist. `cmake --install` writes the program folder: everything next to `calibly.exe` (Qt and MinGW DLLs, FFmpeg, `plugins/`, `qml/`), made by `qt_generate_deploy_qml_app_script()` with `windeployqt` options for no software OpenGL and no QML debugging plugins; the Qt Quick Controls styles other than Basic, which Qt Quick Dialogs pulls in, are removed afterwards. TagLib is excluded from installing (`EXCLUDE_FROM_ALL`, so CMake 3.28 is required now). The folder is 138 MB, the installer 34 MB. `installer/calibly.iss` (Inno Setup 6, German and English) installs per user to `%LOCALAPPDATA%\Programs\calibly`, adds a start-menu entry and an optional desktop icon, removes old `plugins/` and `qml/` folders on an update and never touches the library; it shows `LICENSE.txt` as soon as that file exists (step 2). `tools/make-release.ps1` runs configure, build, tests, install and Inno Setup; `tools/test-in-sandbox.ps1` opens Windows Sandbox (without a graphics card, the harder case) with the installer and optionally a folder of test books on its desktop.
Found on the way: PDF covers were black for PDFs without their own page background (most of them) – the rendered page is transparent there; the cover is now drawn on white (`pdfreader.cpp`, with a test).
Checked: the deployed `calibly.exe` starts with a `PATH` without Qt, imports an EPUB and a PDF and shows them. **Still open:** the test of the installer in Windows Sandbox (needs Windows Sandbox switched on, see the script).

### Step 2 – Licenses
1. **calibly's license**: a proprietary end-user license (`LICENSE.txt`): use free of charge, no redistribution or modification, no warranty, all rights reserved. Shown in the installer and in "Über calibly". (A lawyer should look at the final text if the release reaches many people; the plan only provides the slot.)
2. **Third-party notices** (`THIRD-PARTY-NOTICES.txt`, installed next to the exe and shown in "Über calibly"): name, version, license text and source link of Qt, TagLib, FFmpeg, PDFium, Feather Icons.
3. **LGPL with closed source:**
   - Qt and FFmpeg: allowed because they are separate DLLs the user can replace; the notices say so and link to the Qt source of the used version (6.12.0).
   - **TagLib is linked statically.** LGPL 2.1 would then require shipping calibly's object files for relinking. Two ways out: use TagLib under its second license **MPL 1.1** (allows static linking; only changes to TagLib itself would have to be published – there are none), or build TagLib as a DLL. **Recommendation: MPL 1.1**, written into the notices; switch to a DLL if in doubt.
- **Done when:** `LICENSE.txt` and `THIRD-PARTY-NOTICES.txt` are part of the installer and the app, and every third-party component in the deployed folder is listed.

**Implemented as:** calibly's license terms are `LICENSE.txt` (English) and `LICENSE.de.txt` (German, binding in case of doubt): free use on your own devices; no passing on, changing or reverse engineering beyond what the law allows; third-party parts only under their own licenses, with the LGPL right to replace Qt / FFmpeg and to change and debug calibly for that stated explicitly (LGPL 3, section 4 requires it); "beta" = test version; liability limited to intent and gross negligence (as German law allows for software given away free); privacy (local data only; the update check of step 4 contacts GitHub); German law. The installer shows the file of its language before installing. **The text is a draft and not legal advice** – have it checked before calibly reaches many people.
`THIRD-PARTY-NOTICES.txt` is generated on every `cmake --install` by `tools/generate-third-party-notices.py` (needs Python 3 and the Qt documentation, `CALIBLY_QT_DOCS_DIR`) from the hand-written part `licenses/third-party-notices.in.txt` – Qt (LGPL 3), FFmpeg (LGPL 2.1), TagLib (used under **MPL 1.1**), UTF8-CPP, the MinGW / GCC runtime, Feather Icons – and, for exactly the Qt DLLs and plugins in the folder, the components Qt has built in, taken from Qt's software bill of materials (`<Qt>/sbom/*.spdx.json`) with license texts from Qt's attribution pages: 63 components (PDFium and its Chromium parts, FFmpeg, HarfBuzz, FreeType, PCRE2, SQLite, zlib, …). Qt 6.12's SBOM misses three links (the FFmpeg plugin's implementation, Qt PDF's Chromium parts, bundled libraries such as HarfBuzz without an attribution entry); the script adds them explicitly and **stops the release if a DLL or a component has no license information**, so a new dependency cannot slip through. One license text missing from Qt's documentation (XSVG) is kept in `licenses/qt-extra/`. The full license texts (GPL 3, LGPL 3, LGPL 2.1, MPL 1.1, BSL 1.0, GCC runtime exception, MinGW-w64 runtime, winpthreads) come from the MinGW and TagLib sources into `licenses/texts/` and are installed to `licenses/`.
Source code of the LGPL libraries: the notices link to the sources and promise the exact sources of the shipped versions on request for three years (via the issues of the releases repository) – step 5 should check whether that is enough or whether the Qt and FFmpeg sources should be attached to each release.
On the way, the installed folder lost five SQL drivers calibly never uses (Oracle, PostgreSQL, ODBC, InterBase, Mimer). "Über calibly" (step 4) will show both files.

### Step 3 – Robustness
1. **Library cannot be opened** (folder gone, e.g. an unplugged drive; database damaged; created by a newer calibly): no sample data any more. A message says what happened and offers "Erneut versuchen", "Anderen Ordner wählen" and "Neue Bibliothek anlegen"; a damaged database is never overwritten.
2. **One instance only** (`QLockFile` in the settings folder): a second start brings the running window to the front.
3. **Backup before a migration**: `calibly.db` is copied to `calibly-<old version>.db.bak` before the schema changes; the last three backups are kept.
4. **Log file** in `%LOCALAPPDATA%\calibly\logs` (Qt messages via `qInstallMessageHandler`, one file per day, kept for 14 days). "Über calibly" opens the folder, for bug reports.
5. **Large library**: try 2,000 works (a generated test library) for start time, scrolling, search and memory; fix what is slow.
6. **First start**: check the empty library (the text "Noch keine Titel in der Bibliothek." exists) – it should point to "Hinzufügen" and drag & drop, and say where the library folder is.
7. **Private Qt API**: keep `QZipReader` (it works and is tested), but note the Qt version in the README; a test in `tst_reader` / `tst_import` catches a change when Qt is updated.
- **Done when:** the cases above are covered by tests where possible (backup, lock, opening errors) and tried by hand otherwise.

**Implemented as:**
- *Opening the library:* `SqliteBackend::open(error, create)` tells why it failed (`OpenError`: not found, cannot create, damaged – checked with `PRAGMA quick_check` – newer version, update failed) and creates a library only when asked to. `LibraryController` can now open a library at any time (`openLibrary(folder, create)`, also for the settings page of step 4): it locks the folder, opens it, lets the player and the reader go of the old works and remembers the folder for the next start. If the other library cannot be opened, the open one stays. `main.cpp` opens the last used library without creating it again (a new one only on the very first start or with `--library`); if it fails, there is **no sample data any more**: the window shows `LibraryProblem` (new component) with what happened, the folder, "Erneut versuchen", "Andere Bibliothek öffnen …" and "Neue Bibliothek anlegen …" (a `FolderDialog`). A damaged database is never written to.
- *Backup:* before a schema update calibly.db is copied to `calibly.db.v<old version>-<date>.bak` in the library folder; the newest three are kept.
- *One calibly per library* (`LibraryLock`): a lock file `calibly.lock` in the library folder (`QLockFile`; a lock left by a crashed calibly is taken over) – per library rather than per app, so a second library or a development build can run alongside. The holder listens on a `QLocalServer`; a second start with the same library calls it, allows it to come to the front (`AllowSetForegroundWindow`) and ends; the running window raises itself (`activationRequested`). New dependency: Qt Network (already deployed by Qt).
- *Log file* (`LogFile`, core): every Qt message also goes to `%LOCALAPPDATA%\calibly\logs\calibly-<date>.log`, one file per day, older than 14 days deleted at start; the console output stays.
- *First start:* the empty library already had "E-Books und Hörbücher hinzufügen"; it now also says that files and folders can be dragged in and where the library is.
- *Large library:* a generated library of 2,000 works (half with an audiobook, a third in series, covers) opens its window after 0.66 s instead of 0.45 s for an empty one, using about 170 MB instead of 113 MB (release build). Scrolling has to be judged by hand.
- *QZipReader:* stays; the README names Qt 6.12.0, and the EPUB tests of `tst_import` and `tst_reader` would catch a change of the private API.
- Tests: `tst_import` (backups, the three opening errors, the database left unchanged), `tst_librarycontroller` (open, create, switch, error keeps the open library, lock and activation, remembered folder), new `tst_logfile`.

### Step 4 – Settings page and update hint (for the beta)
1. **Settings** as a view (sidebar entry at the bottom, gear icon, or a button next to the logo), new components first in the gallery:
   - **Bibliothek**: the folder (shown, "Im Explorer öffnen"), "Anderen Ordner wählen" (opens an existing library or creates a new one; files are **not** moved – moving a library is not part of 1.0).
   - **Sprache**: Deutsch / English / like the system (the `LanguageManager` exists, only the UI is missing).
   - **Updates**: "Beim Start nach neuen Versionen suchen" (on by default), "Jetzt suchen".
   - **Über calibly**: version, license, third-party notices, log folder, link to the releases page.
2. **Update hint**: at most once a day calibly asks `https://api.github.com/repos/mblees/calibly-releases/releases/latest` (no account, no personal data; pre-releases count while a beta is installed). If the version is newer, a quiet hint (like the import queue) says "calibly 0.2 ist verfügbar" with "Herunterladen" (opens the release page) and "Ignorieren". No connection = no message.
3. **Hide the placeholders** "Autoren" and "Sprecher" in the beta, so the beta has nothing unfinished (they return in step 6).
- **Done when:** language, library folder and update check can be changed without command-line options, and a test release in the releases repository is found by the app.

**Implemented as:**
- *Settings* (`SettingsView`, the entry "Einstellungen" at the bottom of the sidebar – `SidebarNav` got `bottomModel` for it): **Bibliothek** – the folder, "Im Explorer öffnen", "Andere Bibliothek öffnen …" / "Neue Bibliothek anlegen …" (`LibraryController.openLibraryUrl()` from step 3; an error is shown below the buttons and the open library stays); **Sprache** – "Wie das System", Deutsch, English (`LanguageManager.followSystem()` / `chooseLanguage()`; the system's language can now also be chosen on purpose); **Neue Versionen** – the switch, "Jetzt suchen" with the result, and what is sent; **Über calibly** – version and links to the license terms, the third-party notices, the log folder and the download page (`AppInfo`; license and notices exist only in an installed calibly and are greyed out in a development build).
- *Update check* (`UpdateChecker`): at start (3 s after, at most once a day, only with a real library) and with "Jetzt suchen" calibly asks `api.github.com/repos/<CALIBLY_RELEASES_REPO>/releases` (no account, a `User-Agent` "calibly/<version>"). Drafts never count, pre-releases only while a pre-release is installed; versions compare numerically ("0.10" > "0.9", "1.0.0" > "1.0.0-beta.2" > "1.0.0-beta"). A newer version shows `UpdateHint` at the bottom left (not in fullscreen) with "Herunterladen" (the release page) and "Ignorieren" (no hint for that version again; the settings still show it). No connection only shows on the settings page; the reason is logged. Checked with the installed build: the request reaches GitHub over HTTPS (404 until the releases repository exists).
- *Placeholders:* "Autoren" and "Sprecher" are left out of the sidebar until step 6.
- Tests: new `tst_appsettings` (version comparison, choice of the release, hint and "Ignorieren", stored switch, language: system / chosen).
- **Still open:** the releases repository `mblees/calibly-releases` does not exist yet (step 5), so "a test release is found by the app" can only be checked then.

### Step 5 – 0.1 Beta
1. Create the public repository `mblees/calibly-releases`: a user README (German and English: what calibly is, screenshots, system requirements Windows 10/11 64-bit, install, where the library lives, how to report bugs), issue templates (bug report asks for version and log file).
2. **Code signing** (open decision, see section 4): without a signature, Windows SmartScreen warns "Der Computer wurde durch Windows geschützt" at the first downloads; the README explains "Weitere Informationen → Trotzdem ausführen".
3. Manual test with the checklist (section 5) on the developer PC and in Windows Sandbox.
4. Tag `v0.1.0-beta` in the private repository, GitHub **pre-release** in the releases repository with the setup file, release notes and known issues.
- **Done when:** the beta can be downloaded from the public page and installed, and the app of an older test build shows the update hint for it.

### Step 6 – Towards 1.0
1. **Beta feedback**: fix reported bugs, each with a test where possible; further betas (`0.2.0-beta` …) as needed.
2. **Views "Autoren" and "Sprecher"** (separate small plan, like the phases of the UI plan): a list of all authors / narrators with the number of titles and a small cover row, a click shows their titles in the cover grid (as the series do). Sidebar counts.
3. Feature freeze: from here on only fixes.
4. Translations complete (`update_translations`, no unfinished entries), README and release notes in both languages.
5. Full checklist (section 5) on a clean Windows, then version `1.0.0`, tag `v1.0.0`, release (not a pre-release).
- **Done when:** 1.0 is published and no known bug that loses data or blocks reading or listening is open.

## 4. Open decisions

| Question | Recommendation | Latest by |
|---|---|---|
| Code signing | Start **unsigned** for the beta (explained in the README). For 1.0 consider **Azure Trusted Signing** (about 10 $ per month, needs identity verification) – much cheaper than a classic certificate (several hundred € per year). | Step 6 |
| ~~Name of the releases repository~~ | **Decided:** the working repository `mblees/calibly` (`CALIBLY_RELEASES_REPO`) | done |
| TagLib: MPL 1.1 or DLL | MPL 1.1 (no build change) | Step 2 |
| Text of the end-user license | Short proprietary license as above; have it checked if calibly spreads widely | Step 2 |
| Crash reports | Only the log file, sent by the user with a bug report – no automatic reporting (no data leaves the PC unasked) | Step 3 |
| Windows 10 | Support it if the clean-test on Windows 10 passes (Qt 6.12 still runs there); otherwise state "Windows 11" | Step 5 |

## 5. Release checklist (manual)

Run before every release, on the developer PC and in Windows Sandbox:
- Install, start, uninstall; reinstall over the old version keeps library and settings.
- First start with an empty library; drag & drop and "Hinzufügen" of EPUB, PDF, MOBI, TXT, M4B, an MP3 folder; drop on a cover.
- Detail view, metadata editor (save, ‹ ›), cover states, "Weitermachen", series, selection, merge, remove.
- Player: play, skip, chapters, speed, sleep timer, position after restart.
- Reader: EPUB and PDF, turning pages (keys, wheel, click zones), contents, links, text settings, fullscreen and back (also from a maximized window), Esc order, position after restart and after changing the font size, finished at the last page.
- Settings: language switch, other library folder, update check (with a test release).
- Error cases: library folder missing, a file deleted outside calibly, damaged EPUB, second instance.
- English UI: no German text left.
