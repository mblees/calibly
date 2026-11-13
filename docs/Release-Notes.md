# Release Notes

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

## 0.1.0-beta

First public beta: library for ebooks and audiobooks with import by drag & drop, metadata editor, audiobook player, ebook reader (EPUB, PDF), "Continue", series stacks, multi-selection, installer, settings and update hint.
