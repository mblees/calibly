# calibly
One library for ebooks and audiobooks - calibre's library management combined with an Audibly-style audiobook player.

## Compatability
- Main Intent: Windows 11
> Should be developed with later Linux compatability in mind. Implementation in Adroid and IOS thinkable in the future. 

## Functions
- Classical calibre interface which also accepts audiobooks.
- Modify the metadata of audiobooks and ebooks the same way you can modify ebooks in calibre.
- Integrated player for audiobooks similar to Audibly.
- Integrated reader for ebooks similar to calibre.
- Importing ebooks and audiobooks by pulling the files in the interface.

## UI Mockup
Prototype of the planned cover gallery interface (not implemented yet): main library view, detail view, fullscreen player, drag & drop import, metadata editor and cover states.

![calibly cover gallery mockup](mockups/calibly-cover-galerie-v2.png)

## UI Development Plan
The UI will be built with Qt Quick, in small phases that each end with a runnable app. The [UI Development Plan](docs/UI-Development-Plan.md) describes the concept, the modular architecture and the phases.
