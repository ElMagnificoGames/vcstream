import QtQuick 2.15

import QtQuick.Controls 2.15

Item {
    id: root

    property Item target
    property Item overlay
    property string text
    property int maxWidth: 360
    property int margin: 10
    property int offsetY: 6
    property var appPalette
    property var theme

    visible: false
    z: 10000

    SystemPalette { id: sysPal }
    readonly property var pal: ( appPalette ? appPalette : sysPal )

    readonly property color baseColour: ( theme ? theme.baseColour : pal.base )
    readonly property color borderColour: ( theme ? theme.frameColour : pal.mid )
    readonly property color textColour: ( theme ? theme.textColour : pal.text )

    width: Math.min( maxWidth, textItem.implicitWidth + 16 )
    height: textItem.implicitHeight + 16

    Rectangle {
        anchors.fill: parent
        radius: ( theme ? theme.panelRadius : 6 )
        color: ( theme ? theme.panelColour : baseColour )
        border.color: borderColour
        border.width: ( theme ? theme.lineMed : 1 )
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: ( theme ? theme.insetGap : 3 )
        radius: Math.max( 0, ( theme ? theme.panelRadius : 6 ) - 2 )
        color: "transparent"
        border.color: ( theme ? theme.frameInnerColour : borderColour )
        border.width: 1
    }

    Text {
        id: textItem
        anchors.fill: parent
        anchors.margins: 8
        text: root.text
        wrapMode: Text.Wrap
        color: textColour
        width: Math.min( root.maxWidth - 16, implicitWidth )
    }

    function show() {
        if ( !target || !overlay || text.length === 0 ) {
            return
        }

        const p = target.mapToItem( overlay, 0, target.height )

        x = Math.min( p.x, overlay.width - width - margin )
        y = Math.min( p.y + offsetY, overlay.height - height - margin )

        if ( x < margin ) {
            x = margin
        }
        if ( y < margin ) {
            y = margin
        }

        visible = true
    }

    function hide() {
        visible = false
    }
}
