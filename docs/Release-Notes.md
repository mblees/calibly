# Release Notes

## 0.4.0-beta (2026-11-13)

### Deutsch

**Neu**
- **Autoren:** Neue Ansicht in der Seitenleiste. Unter „Anordnung“ wählst du zwischen „Stapel“ (jeder Autor ein Stapel; ein Klick zeigt seine Titel, Serien wieder als Stapel) und „Alle Titel“ (jeder Autor als Überschrift, darunter alle seine Werke einzeln mit Cover). Ein Stapel zeigt das Cover des ersten Bands der ersten Serie oder des ersten Einzeltitels.
- **Neu:** Neue Ansicht mit den Titeln, die in den letzten 10 Minuten hinzugefügt wurden, die neuesten zuerst. Metadaten bearbeiten oder einen Titel öffnen macht ihn nicht wieder neu.
- **Reihenfolge einer Serie festlegen:** Haben Titel beim „Als Serie zusammenfassen“ keine Bandnummern, fehlen Nummern oder kommen sie doppelt vor, klickst du die Bände in ihrer Reihenfolge an; die Nummer erscheint auf dem Cover. „Überspringen“ nummeriert wie bisher automatisch.
- **Aus „Weitermachen“ entfernen:** Rechtsklick auf eine Karte unter „Weitermachen“ (oben in der Bibliothek und in der eigenen Ansicht). Der Lese- bzw. Hörstand dieses Formats wird auf den Anfang gesetzt.
- **Entf-Taste:** Entfernt die markierten Titel, nach derselben Rückfrage wie der Knopf „Entfernen“.

**Geändert**
- In den Tabs „E-Books“ und „Hörbücher“ entfernt „Entfernen“ nur das jeweilige Format. Ein Titel mit beiden Formaten bleibt mit dem anderen in der Bibliothek; nur dessen Dateien kommen in den Papierkorb. Unter „Alle“ und „Beides“ wird wie bisher der ganze Titel entfernt.
- Die Seitenleiste ist neu geordnet: Bibliothek, Autoren, Serien, Neu, Weitermachen.
- Im Schriftzug „calibly“ steht „cal“ in der Akzentfarbe.
- Markierte Cover haben einen deutlich sichtbareren Rahmen.

**Behoben**
- „Als nicht fertig markieren“ zeigte einen Titel danach mit 0 % unter „Weitermachen“, und der Reader öffnete ein E-Book wieder am Ende.

**Hinweise**
- Die Bibliothek wird nicht umgestellt; ein Update über 0.3 behält Bibliothek und Einstellungen.

### English

**New**
- **Authors:** a new view in the sidebar. Under "Layout" you choose between "Stacks" (each author is a stack; a click shows their titles, series again as stacks) and "All titles" (each author as a heading with all their works below, one by one with their covers). A stack shows the cover of the first volume of the first series or of the first single title.
- **New:** a new view with the titles added in the last 10 minutes, newest first. Editing the metadata or opening a title does not make it new again.
- **Choose the order of a series:** if titles have no volume numbers when you "Group as series", or numbers are missing or appear twice, you click the volumes in their order; the number appears on the cover. "Skip" numbers them automatically as before.
- **Remove from "Continue":** right click a card under "Continue" (at the top of the library and in its own view). The reading or listening position of that format goes back to the start.
- **Delete key:** removes the selected titles, after the same question as the "Remove" button.

**Changed**
- In the tabs "Ebooks" and "Audiobooks", "Remove" only removes that format. A title with both formats stays in the library with the other one; only the removed format's files go to the recycle bin. Under "All" and "Both" the whole title is removed as before.
- The sidebar has a new order: Library, Authors, Series, New, Continue.
- In the wordmark "calibly", "cal" is in the accent color.
- Selected covers have a clearly visible frame.

**Fixed**
- "Mark as not finished" then showed a title at 0 % under "Continue", and the reader opened an ebook at its end again.

**Notes**
- The library is not updated; updating over 0.3 keeps the library and the settings.

## 0.3.0-beta (2026-11-13)

### Deutsch

**Neu**
- **calibly ist Open Source:** calibly steht jetzt unter der GNU General Public License Version 3 (GPL-3.0-only). Du darfst calibly unter ihren Bedingungen weitergeben und verändern. Der Quellcode liegt auf GitHub: https://github.com/mblees/calibly
- **Über calibly** (Einstellungen) nennt Copyright, Lizenz und dass calibly ohne Gewährleistung bereitgestellt wird, mit den Links „Lizenz (GNU GPL 3)“ und „Quellcode“.

**Geändert**
- Der Installer zeigt die GPL 3 in beiden Sprachen auf Englisch, weil nur der englische Text verbindlich ist. Die bisherigen deutschen Lizenzbedingungen (`LICENSE.de.txt`) entfallen und werden beim Update entfernt.
- Die Hinweise zu Software Dritter nennen den Quellcode von calibly; TagLib wird jetzt unter der LGPL 2.1 genutzt (bisher MPL 1.1).

**Hinweise**
- Sonst hat sich nichts geändert. Ein Update über 0.2 behält Bibliothek und Einstellungen, die Bibliothek wird nicht umgestellt.

### English

**New**
- **calibly is open source:** calibly is now licensed under the GNU General Public License version 3 (GPL-3.0-only). You may redistribute and modify it under its terms. The source code is on GitHub: https://github.com/mblees/calibly
- **About calibly** (settings) names the copyright, the license and that calibly comes without any warranty, with the links "License (GNU GPL 3)" and "Source code".

**Changed**
- The installer shows the GPL 3 in English in both languages, because only the English text is official. The former German license terms (`LICENSE.de.txt`) are gone and are removed by the update.
- The third-party notices name calibly's source code; TagLib is now used under the LGPL 2.1 (before: MPL 1.1).

**Notes**
- Nothing else changed. Updating over 0.2 keeps the library and the settings; the library is not updated.

## 0.2.0-beta (2026-11-13)

### Deutsch

**Neu**
- **Neues App-Icon:** Buchrücken auf einem Regal, deren Höhen einen Equalizer bilden. Der Schriftzug in der Seitenleiste ist jetzt ruhig und einfarbig.
- **Akzentfarbe wählen:** In den Einstellungen unter „Akzentfarbe“ mit Farbrad, Helligkeit und acht Schnellfarben. Die Farbe gilt für Schaltflächen, Auswahl und alles rund ums Hören; E-Books bleiben blau.
- **Exportieren:** Markierte Titel lassen sich in einen Ordner deiner Wahl kopieren, aufgeteilt in `Ebooks` und `Audiobooks`, ohne weitere Unterordner. Der Fortschritt steht unten links, „Abbrechen“ löscht den angefangenen Export wieder.
- **Als Serie zusammenfassen:** Markierte Titel bekommen einen gemeinsamen Seriennamen. Vorhandene Bandnummern bleiben, fehlende werden vergeben. Eine Serie lässt sich über „Serie auflösen …“ (Rechtsklick auf den Stapel) wieder auflösen, einzelne Titel mit „Aus der Serie nehmen“.
- **Kombinieren:** Ersetzt „Zu einem Werk verbinden“. Ein kombinierter Eintrag liest sich am Stück: „Lesen“ öffnet den aktuellen Band an der gespeicherten Stelle, am Ende eines Bands geht es im nächsten weiter. Hörbücher laufen nacheinander. „Kombination auflösen“ (Rechtsklick oder Detailansicht) macht wieder einzelne Titel daraus, mit ihren Titeln, Bandnummern und Lesepositionen.
- **Alle als fertig / neu markieren** für die markierten Titel (Leiste → „⋯“).
- **Serien-Stapel markieren:** Ein Stapel lässt sich wie ein einzelner Titel markieren und steht dann für alle seine Bände. Stapel, Einzeltitel und kombinierte Einträge können gemeinsam markiert werden.

**Geändert**
- Während Titel markiert sind, markiert ein Klick auf ein Cover den Titel, statt die Details zu öffnen. Die Knöpfe „Lesen“ / „Hören“ auf den Covern sind dann ausgeblendet.
- „Metadaten“ in der Auswahl ist nur bei genau einem Titel aktiv.
- Der Import nimmt nur noch Bücher (EPUB, PDF, MOBI, AZW3) und Hörbücher (M4B, M4A, MP3, AAC, OGG, Opus, FLAC) an. Text- und Markdown-Dateien werden nicht mehr importiert; vorhandene öffnet der Reader weiterhin.

**Behoben**
- Bei verbundenen Werken mit mehreren Bänden öffnete „Lesen“ immer Band 1, und „Als gelesen markieren“ wirkte auf den falschen Band.
- Ein einziger fertiger Band zeigte das ganze Werk als fertig.

**Hinweise**
- Die Bibliothek wird beim ersten Start auf Version 6 umgestellt. Vorher legt calibly eine Sicherung `calibly.db.v5-….bak` im Bibliotheksordner an. Ältere calibly-Versionen können die umgestellte Bibliothek danach nicht mehr öffnen.
- Werke, die schon mit 0.1 zu einem Werk verbunden wurden, verhalten sich weiter wie ein Buch. Damit sie als Bände gelten, einmal „Kombination auflösen“ und die Titel dann neu „Kombinieren“.

### English

**New**
- **New app icon:** book spines on a shelf whose heights form an equalizer. The wordmark in the sidebar is now calm and in one color.
- **Choose the accent color:** in the settings under "Akzentfarbe", with a color wheel, brightness and eight quick choices. It colors buttons, the selection and everything about listening; ebooks stay blue.
- **Export:** selected titles can be copied into a folder of your choice, split into `Ebooks` and `Audiobooks`, without further subfolders. The progress shows at the bottom left; "Cancel" deletes the unfinished export again.
- **Group as series:** selected titles get one series name. Existing volume numbers stay, missing ones are given. A series can be dissolved again with "Dissolve series …" (right click on the stack), single titles with "Remove from series".
- **Combine:** replaces "Merge into one work". A combined entry reads in one go: "Read" opens the current volume where you stopped, and at the end of a volume reading goes on in the next one. Audiobooks play one after the other. "Dissolve combination" (right click or detail view) turns it back into single titles with their titles, volume numbers and reading positions.
- **Mark all as finished / new** for the selected titles (bar → "⋯").
- **Select series stacks:** a stack can be selected like a single title and stands for all its volumes. Stacks, single titles and combined entries can be selected together.

**Changed**
- While titles are selected, a click on a cover selects it instead of opening the details. The "Read" / "Listen" buttons on the covers are hidden then.
- "Metadata" in the selection is enabled for exactly one title only.
- The import only accepts books (EPUB, PDF, MOBI, AZW3) and audiobooks (M4B, M4A, MP3, AAC, OGG, Opus, FLAC). Text and Markdown files are no longer imported; the reader still opens those a library already has.

**Fixed**
- In merged works with several volumes, "Read" always opened volume 1 and "Mark as read" changed the wrong volume.
- A single finished volume showed the whole work as finished.

**Notes**
- The library is updated to version 6 at the first start. calibly writes a backup `calibly.db.v5-….bak` into the library folder first. Older calibly versions cannot open the updated library.
- Works merged with 0.1 still behave like one book. To make them volumes, use "Dissolve combination" once and then "Combine" the titles again.

## 0.1.0-beta (2026-11-12)

Erste Testversion von calibly: eine Bibliothek für E-Books und Hörbücher, mit Hörbuch-Player und E-Book-Reader.

### Deutsch

**Installieren**
- `calibly-0.1.0-beta-setup.exe` herunterladen und starten. calibly wird nur für den aktuellen Benutzer installiert, Administratorrechte sind nicht nötig. Voraussetzung ist Windows 11 (64 Bit).
- Der Installer ist nicht signiert. Windows SmartScreen meldet deshalb „Der Computer wurde durch Windows geschützt“; über „Weitere Informationen“ → „Trotzdem ausführen“ geht es weiter.
- Beim Deinstallieren bleibt die Bibliothek erhalten.

**Bibliothek**
- Cover-Wand mit Suche (Titel, Autor, Sprecher, Serie), Auswahl nach Format (E-Books, Hörbücher, beides), Sortierung und einstellbarer Cover-Größe.
- Die Bände einer Serie liegen als Stapel zusammen; „Serien“ in der Seitenleiste listet alle Serien.
- **Weitermachen:** Die zuletzt geöffneten, nicht fertigen Titel stehen oben in der Bibliothek und als eigene Ansicht in der Seitenleiste.
- Kennzeichen auf den Covern: „NEU“, „FERTIG“ und „DATEI FEHLT“.

**Importieren**
- Dateien oder ganze Ordner ins Fenster ziehen oder über „Hinzufügen“ wählen: EPUB, PDF, MOBI, Text, Markdown, M4B, MP3 und weitere Audioformate.
- Auf ein Cover gezogen wird die Datei zu einem weiteren Format dieses Titels, z. B. das Hörbuch zum E-Book. Die MP3-Dateien eines Ordners werden ein Hörbuch.
- Die Dateien werden in den Bibliotheksordner `Dokumente\calibly-Bibliothek` kopiert, sortiert nach Autor und Titel.
- Die Import-Liste unten rechts zeigt jeden Titel; „Prüfen“ öffnet die Metadaten von Titeln, deren Name nur aus dem Dateinamen stammt.

**Hören**
- Player-Leiste am unteren Rand und Vollbild-Player mit Kapitelliste, Tempo und Schlaftimer.
- Leertaste: Wiedergabe / Pause, ← / →: 30 Sekunden zurück / vor.
- Die Position wird gespeichert, das letzte Hörbuch ist nach einem Neustart wieder bereit.

**Lesen**
- Reader im Fenster für EPUB, PDF, Text und Markdown, mit Inhaltsverzeichnis.
- „Aa“ öffnet die Texteinstellungen: Größe, Themen, Seitenfarbe, Schrift, Abstände, Ränder, Blocksatz; bei PDFs der Zoom.
- Vollbild mit F11, Blättern mit Pfeiltasten, Bild ↑/↓, Leertaste, Mausrad oder Klick an den Seitenrand.
- Die Stelle wird gespeichert; die letzte Seite markiert das E-Book als fertig.

**Bearbeiten und Auswählen**
- Detailansicht mit einer Karte pro Format, „Weiterlesen“ / „Weiterhören“ und „Als fertig markieren“.
- Metadaten-Editor: Felder mit 🔗 gelten für alle Formate eines Titels. Strg+S speichert, Strg+Bild ↑/↓ wechselt zum vorigen / nächsten Titel.
- Mehrfachauswahl mit dem Kreis auf dem Cover, Strg+Klick oder Strg+A: Metadaten nacheinander bearbeiten, „Zu einem Werk verbinden“ oder entfernen (die Dateien wandern in den Papierkorb).
- Rechtsklick auf ein Cover öffnet ein Menü mit denselben Aktionen.

**Einstellungen**
- Bibliothek: im Explorer öffnen, eine andere Bibliothek öffnen oder eine neue anlegen (es werden keine Dateien verschoben).
- Sprache: wie das System, Deutsch oder English.
- Neue Versionen: höchstens einmal am Tag bei GitHub nachsehen, es werden keine persönlichen Daten gesendet.
- Über calibly: Version, Lizenzbedingungen, Hinweise zu Software Dritter und Log-Dateien.

**Sicherheit für deine Daten**
- Ist die Bibliothek nicht erreichbar (z. B. ein nicht angeschlossenes Laufwerk) oder beschädigt, sagt calibly das und bietet „Erneut versuchen“, eine andere oder eine neue Bibliothek an. Eine bisherige Bibliothek wird nie stillschweigend ersetzt.
- Bevor eine neue Version die Datenbank umstellt, legt calibly eine Sicherung `calibly.db.v<alte Version>-<Datum>.bak` an; die neuesten drei bleiben.
- Mit einer Bibliothek arbeitet immer nur ein calibly; ein zweiter Start holt das offene Fenster nach vorn.
- Log-Dateien unter `%LOCALAPPDATA%\calibly\logs`, eine pro Tag, 14 Tage lang.

**Bekannte Einschränkungen**
- MOBI und AZW3 öffnen im Standardprogramm.
- „Zu einem Werk verbinden“ lässt sich nicht rückgängig machen. Ab 0.2 heißt es „Kombinieren“ und lässt sich wieder auflösen.
- Der Hinweis auf neue Versionen funktioniert in der geschlossenen Beta nicht, weil das Repository privat ist.

**Fehler melden:** Bitte mit Version und der Log-Datei des Tages (Einstellungen → Über calibly → Log-Dateien).

### English

**Install**
- Download and run `calibly-0.1.0-beta-setup.exe`. calibly is installed for the current user only, no admin rights needed. It requires Windows 11 (64-bit).
- The installer is not signed, so Windows SmartScreen says "Windows protected your PC"; continue with "More info" → "Run anyway".
- Uninstalling keeps the library.

**Library**
- Cover wall with search (title, author, narrator, series), format selection (ebooks, audiobooks, both), sorting and adjustable cover size.
- The volumes of a series are shown as one stack; "Series" in the sidebar lists all series.
- **Continue:** the titles opened last that are not finished are shown at the top of the library and as a view of their own in the sidebar.
- Labels on the covers: "NEW", "FINISHED" and "FILE MISSING".

**Import**
- Drag files or whole folders into the window, or pick them with "Add": EPUB, PDF, MOBI, text, Markdown, M4B, MP3 and other audio formats.
- Dropped on a cover, a file becomes another format of that title, e.g. the audiobook of an ebook. The MP3 files of one folder become one audiobook.
- The files are copied into the library folder `Documents\calibly-Bibliothek`, sorted by author and title.
- The import queue at the bottom right shows each title; "Review" opens the metadata of titles whose name could only be taken from the file name.

**Listen**
- Player bar at the bottom and a fullscreen player with the chapter list, speed and sleep timer.
- Space: play / pause, ← / →: 30 seconds back / forward.
- The position is saved, and the last audiobook is ready again after a restart.

**Read**
- Reader inside the window for EPUB, PDF, text and Markdown, with the contents.
- "Aa" opens the text settings: size, themes, page color, font, spacing, margins, justified text; for PDFs the zoom.
- Fullscreen with F11; turn pages with the arrow keys, PgUp / PgDn, Space, the mouse wheel or a click on the edge of the page.
- The place is saved; the last page marks the ebook as finished.

**Edit and select**
- Detail view with one card per format, "Continue reading" / "Continue listening" and "Mark as finished".
- Metadata editor: fields marked with 🔗 apply to all formats of a title. Ctrl+S saves, Ctrl+PgUp / PgDn go to the previous / next title.
- Multi-selection with the circle on a cover, Ctrl + click or Ctrl + A: edit the metadata one after the other, "Merge into one work" or remove (the files go to the recycle bin).
- A right click on a cover opens a menu with the same actions.

**Settings**
- Library: show in Explorer, open another library or create a new one (no files are moved).
- Language: like the system, German or English.
- New versions: looks on GitHub at most once a day; no personal data is sent.
- About calibly: version, license terms, third-party notices and log files.

**Safety for your data**
- If the library cannot be reached (e.g. a drive that is not connected) or is damaged, calibly says so and offers "Try again", another library or a new one. A library that was used before is never replaced silently.
- Before a new version changes the database, calibly writes a backup `calibly.db.v<old version>-<date>.bak`; the newest three are kept.
- Only one calibly works with a library at a time; starting it again brings the open window to the front.
- Log files in `%LOCALAPPDATA%\calibly\logs`, one per day, kept for 14 days.

**Known limitations**
- MOBI and AZW3 open in the default application.
- "Merge into one work" cannot be undone. From 0.2 on it is called "Combine" and can be dissolved again.
- The update hint does not work during the closed beta, because the repository is private.

**Reporting bugs:** please include the version and the log file of the day (Settings → About calibly → Log files).

SHA-256 of the installer: `2F8486E982775870C0C52AEA2BE90126C7D33AC41C556816DD8C9714551984A9`
