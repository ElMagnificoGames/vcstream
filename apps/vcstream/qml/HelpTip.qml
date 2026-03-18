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

    visible: false
    z: 10000

    SystemPalette {
        id: pal
    }

    width: Math.min( maxWidth, textItem.implicitWidth + 16 )
    height: textItem.implicitHeight + 16

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: pal.base
        border.color: pal.mid
        border.width: 1
    }

    Text {
        id: textItem
        anchors.fill: parent
        anchors.margins: 8
        text: root.text
        wrapMode: Text.Wrap
        color: pal.text
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
