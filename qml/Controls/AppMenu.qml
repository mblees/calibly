pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Calibly.Theme

// Menu that opens at the pointer, e.g. on a right click. Its entries are
// AppMenuItem children; AppMenuSeparator draws a line between groups.
// Entries that are not visible take no space. Arrow keys, Enter and Esc
// work as in every menu.
//
//   AppMenu {
//       id: menu
//       AppMenuItem { text: "Bearbeiten"; iconName: "edit"; onTriggered: … }
//       AppMenuSeparator {}
//       AppMenuItem { text: "Entfernen"; iconName: "trash"; danger: true; onTriggered: … }
//   }
//   menu.popup(item, x, y)   // opens at this position of the item
//
// Functions (inherited from Menu): popup(parent, x, y), close()
T.Menu {
    id: control

    // The entries have different widths; the menu is as wide as the widest
    // one. Worked out when the menu opens, from all entries: which of them
    // are visible is only known once the menu is shown.
    property real entryWidth: 0

    function widestEntry(): real {
        let widest = 0
        for (let i = 0; i < control.count; ++i) {
            const entry = control.itemAt(i)
            if (entry)
                widest = Math.max(widest, entry.implicitWidth)
        }
        return widest
    }

    implicitWidth: Math.max(Theme.menuMinWidth, control.entryWidth + leftPadding + rightPadding)
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    padding: Theme.spacingXs
    margins: Theme.spacingS // stays inside the window

    onAboutToShow: control.entryWidth = control.widestEntry()

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationFast }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
    }

    contentItem: ListView {
        implicitHeight: contentHeight
        model: control.contentModel
        interactive: false
        currentIndex: control.currentIndex
    }

    background: Rectangle {
        radius: Theme.radiusM
        color: Theme.surfaceRaised
        border.width: Theme.borderWidth
        border.color: Theme.inputBorder
    }
}
