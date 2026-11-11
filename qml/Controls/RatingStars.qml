pragma ComponentBehavior: Bound
import QtQuick
import Calibly.Theme

// Read-only rating as five stars.
//
// Properties:
//   value  number of filled stars, 0 – 5
Row {
    id: root

    property int value: 0

    spacing: Theme.spacingXxs

    Accessible.role: Accessible.StaticText
    Accessible.name: qsTr("%1 von 5 Sternen").arg(root.value)

    Repeater {
        model: 5

        delegate: Icon {
            required property int index

            name: "star"
            size: Theme.ratingStarSize
            color: index < root.value ? Theme.rating : Theme.textDisabled
        }
    }
}
