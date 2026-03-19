import QtQuick 2.15
import QtQuick.Controls 2.15

ComboBox {
    id: control

    property var theme

    // Avoid accidental value changes while the user is trying to scroll.
    // Wheel scrolling should be handled by the surrounding scroll view.
    wheelEnabled: false

    function vcFindScrollAncestor() {
        var p = control.parent

        while ( p ) {
            // Best-effort: any Flickable-like item should have these.
            if ( p.contentY !== undefined && p.contentHeight !== undefined && p.height !== undefined ) {
                return p
            }
            p = p.parent
        }

        return null
    }

    WheelHandler {
        id: wheelForwarder
        target: control

        // Do not interfere with the popup list; it should scroll normally.
        enabled: !( control.popup && control.popup.visible )
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

        onWheel: function( wheel ) {
            // Intentionally consume the wheel so the ComboBox cannot cycle.
            const scroller = control.vcFindScrollAncestor()
            if ( scroller ) {
                var dy = wheel.pixelDelta.y
                if ( dy === 0 ) {
                    dy = ( wheel.angleDelta.y / 120.0 ) * 40.0
                }

                if ( dy !== 0 ) {
                    const maxY = Math.max( 0, scroller.contentHeight - scroller.height )
                    scroller.contentY = Math.max( 0, Math.min( maxY, scroller.contentY - dy ) )
                }
            }

            wheel.accepted = true
        }
    }

    implicitHeight: ( theme ? theme.controlHeight : 42 )
    leftPadding: 12
    rightPadding: 36
    topPadding: 9
    bottomPadding: 9

    contentItem: Text {
        leftPadding: control.leftPadding
        rightPadding: 0
        text: control.displayText
        font: control.font
        color: ( theme ? theme.textColour : palette.text )
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Canvas {
        x: control.width - width - 12
        y: Math.round( ( control.height - height ) / 2 )
        width: 12
        height: 8
        contextType: "2d"

        onPaint: {
            if ( !context ) {
                return
            }

            context.setTransform( 1, 0, 0, 1, 0, 0 )
            context.clearRect( 0, 0, width, height )

            context.beginPath()
            context.moveTo( 1, 1 )
            context.lineTo( width / 2, height - 1 )
            context.lineTo( width - 1, 1 )

            context.lineWidth = 2
            context.lineJoin = "round"
            context.lineCap = "round"
            context.strokeStyle = ( theme ? theme.primaryAccentColour : palette.highlight )
            context.stroke()
        }
    }

    onThemeChanged: {
        indicator.requestPaint()
    }

    onPaletteChanged: {
        indicator.requestPaint()
    }

    Component.onCompleted: {
        indicator.requestPaint()
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            radius: ( theme ? theme.controlRadius : 10 )
            color: ( theme ? theme.panelInsetColour : palette.base )
            border.color: control.activeFocus
                ? ( theme ? theme.focusRingColour : palette.highlight )
                : ( theme ? theme.frameColour : palette.mid )
            border.width: control.activeFocus ? 2 : 1
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

    popup: Popup {
        y: control.height + 4
        width: control.width
        padding: 6

        background: Rectangle {
            radius: ( theme ? theme.panelRadius : 8 )
            color: ( theme ? theme.panelColour : palette.base )
            border.color: ( theme ? theme.frameColour : palette.mid )
            border.width: ( theme ? theme.lineMed : 1 )

            Rectangle {
                anchors.fill: parent
                anchors.margins: ( theme ? theme.insetGap : 3 )
                radius: Math.max( 0, ( theme ? theme.panelRadius : 8 ) - 2 )
                color: "transparent"
                border.color: ( theme ? theme.frameInnerColour : palette.midlight )
                border.width: 1
            }
        }

        contentItem: ListView {
            id: popupList
            clip: true
            implicitHeight: Math.min( contentHeight, 280 )
            model: control.model
            currentIndex: control.currentIndex

            boundsBehavior: Flickable.StopAtBounds

            delegate: ItemDelegate {
                objectName: ( control.objectName.length > 0
                    ? control.objectName + "PopupDelegate_" + index
                    : "vcComboPopupDelegate_" + index )
                width: popupList.width
                height: 38
                hoverEnabled: true
                highlighted: ( index === control.currentIndex )

                background: Rectangle {
                    radius: ( theme ? theme.controlRadius : 8 )
                    color: ( highlighted || hovered )
                        ? ( theme ? theme.tertiaryAccentColour : palette.highlight )
                        : "transparent"
                }

                contentItem: Text {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    verticalAlignment: Text.AlignVCenter
                    text: {
                        if ( typeof modelData === "string" ) {
                            return modelData
                        }
                        if ( typeof modelData === "number" ) {
                            return String( modelData )
                        }
                        if ( modelData && typeof modelData === "object" ) {
                            if ( control.textRole && modelData[control.textRole] !== undefined ) {
                                return modelData[control.textRole]
                            }
                            if ( modelData.text !== undefined ) {
                                return modelData.text
                            }
                        }
                        if ( control.textRole && model && model[control.textRole] !== undefined ) {
                            return model[control.textRole]
                        }
                        return ""
                    }
                    color: ( theme ? theme.textColour : palette.text )
                    elide: Text.ElideRight
                }

                onClicked: {
                    control.currentIndex = index
                    control.activated( index )
                    control.popup.close()
                }
            }
        }

        VcFlickScrollBar {
            theme: control.theme
            flickable: popupList
            anchors.right: popupList.right
            anchors.top: popupList.top
            anchors.bottom: popupList.bottom
            anchors.margins: 6
        }
    }

    delegate: ItemDelegate {
        objectName: ( control.objectName.length > 0 ? control.objectName + "Delegate_" + index : "vcComboDelegate_" + index )
        width: control.width
        height: 1
        visible: false
        enabled: false
    }
}
