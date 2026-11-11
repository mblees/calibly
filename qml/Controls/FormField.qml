import QtQuick
import Calibly.Theme

// Labeled field of a form: a small label with an optional marker icon above
// the input. The input is the child item; it gets the full width.
//
//   FormField {
//       label: qsTr("Titel")
//       iconName: "link"
//       iconColor: Theme.shared
//       InputField { text: … }
//   }
//
// Properties:
//   label      text above the input
//   iconName   optional marker before the label, a key of Icons.paths
//   iconColor  color of the marker
//   content    the input (default property)
Column {
    id: root

    property string label
    property string iconName
    property color iconColor: Theme.textMuted

    default property alias content: slot.data

    spacing: Theme.spacingXs

    Row {
        spacing: Theme.spacingXs

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.iconName !== ""
            name: root.iconName
            size: Theme.iconSizeSmall
            color: root.iconColor
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.label
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
    }

    // The input fills the width; its height decides the field's height.
    Item {
        id: slot

        width: root.width
        height: childrenRect.height
        implicitHeight: childrenRect.height

        onChildrenChanged: {
            for (let i = 0; i < slot.children.length; ++i)
                slot.children[i].width = Qt.binding(() => slot.width)
        }
    }
}
