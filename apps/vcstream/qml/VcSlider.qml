import QtQuick 2.15
import QtQuick.Controls 2.15

Slider {
    id: control

    property var theme

    implicitHeight: ( theme ? theme.compactControlHeight : 34 )
    leftPadding: 10
    rightPadding: 10

    background: Item {
        readonly property int trackH: ( theme ? Math.max( 6, Math.round( theme.compactControlHeight * 0.26 ) ) : 8 )

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: parent.trackH
            radius: Math.round( height / 2 )
            color: ( theme ? theme.panelInsetColour : palette.base )
            border.color: ( theme ? theme.frameColour : palette.mid )
            border.width: ( theme ? theme.lineThin : 1 )
        }

        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            height: parent.trackH
            width: Math.max( parent.trackH, control.visualPosition * parent.width )
            radius: Math.round( height / 2 )
            color: ( theme ? theme.primaryAccentColour : palette.highlight )
            opacity: control.enabled ? 0.85 : 0.55
        }

        Rectangle {
            visible: control.visualFocus
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: parent.trackH + 8
            radius: Math.round( height / 2 )
            color: "transparent"
            border.color: ( theme ? theme.focusRingColour : palette.highlight )
            border.width: 2
            opacity: 0.9
        }
    }

    handle: Rectangle {
        readonly property int d: ( theme ? Math.max( 18, Math.round( theme.compactControlHeight * 0.70 ) ) : 22 )

        x: control.leftPadding + control.visualPosition * ( control.availableWidth - width )
        y: Math.round( ( control.availableHeight - height ) / 2 )
        width: d
        height: d
        radius: Math.round( d / 2 )
        color: ( theme ? theme.panelColour : palette.button )
        border.color: ( theme ? theme.frameColour : palette.mid )
        border.width: ( theme ? theme.lineMed : 2 )

        Rectangle {
            anchors.centerIn: parent
            width: Math.max( 6, Math.round( parent.width * 0.38 ) )
            height: Math.max( 6, Math.round( parent.height * 0.38 ) )
            radius: Math.round( width / 2 )
            color: ( theme ? theme.primaryAccentColour : palette.highlight )
            opacity: control.pressed ? 0.95 : 0.85
        }
    }
}
