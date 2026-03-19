import QtQuick 2.15
import QtQuick.Controls 2.15

TabButton {
    id: control

    property var theme
    property bool vcStyled: true

    implicitHeight: ( theme ? theme.compactControlHeight + 8 : 42 )
    leftPadding: 12
    rightPadding: 12
    topPadding: 8
    bottomPadding: 8

    contentItem: Text {
        text: control.text
        font: control.font
        color: ( theme
            ? ( control.checked ? theme.textColour : theme.metaTextColour )
            : palette.text )
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            color: control.checked
                ? ( theme ? theme.panelInsetColour : palette.base )
                : "transparent"
            border.color: ( theme ? theme.frameColour : palette.mid )
            border.width: control.checked ? 1 : 0
        }

        Rectangle {
            visible: control.checked
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 4
            color: ( theme ? theme.primaryAccentColour : palette.highlight )
        }
    }
}
