import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
    id: control

    property var theme

    implicitHeight: ( theme ? theme.controlHeight : 42 )
    leftPadding: 12
    rightPadding: 12
    topPadding: 9
    bottomPadding: 9
    selectByMouse: true

    color: ( theme ? theme.textColour : palette.text )
    placeholderTextColor: ( theme ? theme.metaTextColour : palette.placeholderText )

    background: Item {
        Rectangle {
            anchors.fill: parent
            radius: ( theme ? theme.controlRadius : 10 )
            color: ( theme ? theme.panelInsetColour : palette.base )
            border.color: ( control.activeFocus
                ? ( theme ? theme.focusRingColour : palette.highlight )
                : ( theme ? theme.frameColour : palette.mid ) )
            border.width: ( control.activeFocus ? 2 : 1 )
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: ( theme ? theme.insetGap : 3 )
            radius: Math.max( 0, ( theme ? theme.controlRadius : 10 ) - 2 )
            color: "transparent"
            border.color: ( theme ? theme.frameInnerColour : palette.midlight )
            border.width: 1
            visible: !control.activeFocus
        }
    }
}
