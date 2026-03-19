import QtQuick 2.15
import QtQuick.Controls 2.15

TabBar {
    id: control

    property var theme
    property bool vcStyled: true

    spacing: 0

    background: Rectangle {
        color: ( theme ? theme.panelColour : palette.base )
        border.color: ( theme ? theme.frameColour : palette.mid )
        border.width: 1
    }
}
