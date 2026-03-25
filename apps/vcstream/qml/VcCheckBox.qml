import QtQuick 2.15
import QtQuick.Controls 2.15

CheckBox {
    id: control

    property var theme

    spacing: 10
    leftPadding: 0
    rightPadding: 0

    indicator: Rectangle {
        implicitWidth: 22
        implicitHeight: 22
        x: control.leftPadding
        y: Math.round( ( control.height - height ) / 2 )
        radius: 5
        color: control.checked
            ? ( theme ? theme.primaryAccentColour : palette.highlight )
            : ( theme ? theme.panelInsetColour : palette.base )
        border.color: control.activeFocus
            ? ( theme ? theme.focusRingColour : palette.highlight )
            : ( theme ? theme.frameColour : palette.mid )
        border.width: control.activeFocus ? 2 : 1

        Text {
            anchors.centerIn: parent
            text: control.checked ? "✓" : ""
            color: ( theme ? theme.highlightedTextColour : palette.highlightedText )
            font.pixelSize: ( theme ? Math.max( 12, Math.round( theme.fontBasePx * 1.0 ) ) : 14 )
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        rightPadding: 0
        text: control.text
        font: control.font
        color: ( theme ? theme.textColour : palette.text )
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }
}
