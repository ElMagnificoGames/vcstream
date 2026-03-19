import QtQuick 2.15

Item {
    id: root

    // A simple, app-owned scrollbar for Flickable-like items.
    // It avoids native style/plugin behaviour and ensures predictable rendering.

    property var theme
    property var flickable
    property bool vcStyled: true

    property int barWidth: 12
    property int minThumbHeight: 28

    width: barWidth

    readonly property real viewportHeight: {
        if ( !flickable || flickable.height === undefined ) {
            return 0
        }
        return flickable.height
    }

    readonly property real contentExtent: {
        if ( !flickable ) {
            return 0
        }

        if ( flickable.contentHeight !== undefined ) {
            if ( flickable.contentHeight > 0 ) {
                return flickable.contentHeight
            }
        }

        if ( flickable.contentItem && flickable.contentItem.childrenRect ) {
            if ( flickable.contentItem.childrenRect.height > 0 ) {
                return flickable.contentItem.childrenRect.height
            }
        }

        if ( flickable.contentItem && flickable.contentItem.height !== undefined ) {
            if ( flickable.contentItem.height > 0 ) {
                return flickable.contentItem.height
            }
        }

        if ( flickable.childrenRect ) {
            return flickable.childrenRect.height
        }

        return 0
    }

    readonly property bool canScroll: {
        return contentExtent > viewportHeight + 1
    }

    visible: canScroll

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: ( theme ? Qt.rgba( theme.panelInsetColour.r, theme.panelInsetColour.g, theme.panelInsetColour.b, 0.30 ) : "transparent" )
        border.color: ( theme ? Qt.rgba( theme.frameColour.r, theme.frameColour.g, theme.frameColour.b, 0.40 ) : "transparent" )
        border.width: ( theme ? 1 : 0 )
    }

    Rectangle {
        id: thumb

        width: parent.width
        radius: 6

        readonly property real ratio: ( root.flickable ? root.flickable.visibleArea.heightRatio : 1.0 )
        readonly property real yPos: {
            if ( !root.flickable ) {
                return 0.0
            }

            if ( root.flickable.visibleArea ) {
                return root.flickable.visibleArea.yPosition
            }

            const range = Math.max( 1, root.contentExtent - root.viewportHeight )
            return root.flickable.contentY / range
        }

        height: Math.max( root.minThumbHeight, parent.height * Math.max( 0.0, Math.min( 1.0, ratio ) ) )
        y: Math.max( 0, Math.min( parent.height - height, parent.height * yPos ) )

        color: ( theme ? theme.frameColour : "#666666" )
        opacity: dragArea.drag.active ? 0.95 : ( dragArea.containsMouse ? 0.85 : 0.75 )

        MouseArea {
            id: dragArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor

            drag.target: thumb
            drag.axis: Drag.YAxis
            drag.minimumY: 0
            drag.maximumY: parent.parent.height - thumb.height

            onPositionChanged: function( mouse ) {
                if ( !root.flickable ) {
                    return
                }
                if ( drag.active ) {
                    const trackRange = Math.max( 1, parent.parent.height - thumb.height )
                    const t = thumb.y / trackRange
                    root.flickable.contentY = t * ( root.contentExtent - root.viewportHeight )
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.visible
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton

        onClicked: function( mouse ) {
            if ( !root.flickable ) {
                return
            }

            const trackRange = Math.max( 1, height - thumb.height )
            const desiredY = Math.max( 0, Math.min( trackRange, mouse.y - thumb.height / 2 ) )
            const t = desiredY / trackRange
            root.flickable.contentY = t * ( root.contentExtent - root.viewportHeight )
        }
    }
}
