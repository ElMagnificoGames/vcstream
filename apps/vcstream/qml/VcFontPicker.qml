import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property var theme
    property var appPalette
    property var fontPreviewSafetyCache
    property string systemFontFamily: ""

    // List of font family names (strings).
    property var families: []

    // Selected value: empty string means system default.
    property string value: ""

    // Sample text rendered in the candidate font.
    property string sampleText: "The quick brown fox jumps over the lazy dog. 0123456789"

    signal valueSelected( string value )

    readonly property var pal: ( appPalette ? appPalette : Qt.application.palette )
    readonly property string resolvedName: ( value && value.length > 0 ) ? value : "System default"

    readonly property int vcPreviewPx: ( theme ? theme.fontSmallPx : 12 )
    readonly property string vcFallbackFamily: ( systemFontFamily && systemFontFamily.length > 0 )
        ? systemFontFamily
        : Qt.application.font.family

    // Bump when the popup opens so visible delegates can request checks.
    property int vcPopupOpenSerial: 0

    // Bump when any font health status changes.
    property int vcHealthSerial: 0

    Connections {
        target: root.fontPreviewSafetyCache

        function onStatusChanged( family, pixelSize, status ) {
            root.vcHealthSerial = root.vcHealthSerial + 1
        }
    }

    implicitHeight: pickerButton.implicitHeight
    implicitWidth: pickerButton.implicitWidth

    height: implicitHeight
    width: implicitWidth

        VcToolButton {
        id: pickerButton
        objectName: ( root.objectName && root.objectName.length > 0 )
            ? ( root.objectName + "Button" )
            : "vcFontPickerButton"
        theme: root.theme
        tone: "neutral"
        text: root.resolvedName
        anchors.fill: parent
        font.family: root.vcFallbackFamily

        // Opening the picker is a multi-step interaction; do it only in targeted tests.
        property bool testSkipActivate: true

        onClicked: {
            pickerPopup.open()
        }
    }

    Popup {
        id: pickerPopup

        parent: Overlay.overlay

        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property int vcMargin: 12
        property int vcBottomExtra: ( theme ? theme.spaceCompact : 8 )
        property int vcGap: ( theme ? theme.spaceNudge : 4 )
        property int vcPreferredListHeight: 360

        function vcClamp( v, lo, hi ) {
            return Math.max( lo, Math.min( hi, v ) )
        }

        function vcReposition() {
            if ( !pickerPopup.parent || !pickerButton ) {
                return
            }

            const p = pickerPopup.parent
            const mTop = vcMargin
            const mBottom = vcMargin + vcBottomExtra
            const gap = vcGap

            const buttonPos = pickerButton.mapToItem( p, 0, 0 )
            const buttonX = buttonPos.x
            const buttonY = buttonPos.y

            const desiredW = Math.min( 560, Math.max( 340, pickerButton.width ) )
            pickerPopup.width = Math.min( desiredW, Math.max( 200, p.width - 2 * mTop ) )

            // We want: search field + spacing + list area (preferred) + padding.
            const paddingPx = pickerPopup.padding
            const searchH = ( theme ? theme.controlHeight : 42 )
            const spacingPx = ( theme ? theme.spaceCompact : 8 )
            const preferredH = paddingPx * 2 + searchH + spacingPx + vcPreferredListHeight

            const belowTop = buttonY + pickerButton.height + gap
            const spaceBelow = ( p.height - mBottom ) - belowTop
            const aboveBottom = buttonY - gap
            const spaceAbove = aboveBottom - mTop

            const openDown = ( spaceBelow >= spaceAbove )
            const maxH = Math.max( 140, openDown ? spaceBelow : spaceAbove )
            pickerPopup.height = Math.min( preferredH, maxH )

            var xWanted = buttonX
            var yWanted = openDown ? belowTop : ( buttonY - gap - pickerPopup.height )

            xWanted = vcClamp( xWanted, mTop, Math.max( mTop, p.width - mTop - pickerPopup.width ) )
            yWanted = vcClamp( yWanted, mTop, Math.max( mTop, p.height - mBottom - pickerPopup.height ) )

            pickerPopup.x = xWanted
            pickerPopup.y = yWanted
        }

        onOpened: {
            vcReposition()
            Qt.callLater( vcReposition )
            root.vcPopupOpenSerial = root.vcPopupOpenSerial + 1
        }

        onWidthChanged: {
            // Ensure internal layout gets a chance to settle too.
            Qt.callLater( vcReposition )
        }

        Connections {
            target: pickerPopup.parent

            function onWidthChanged() {
                pickerPopup.vcReposition()
            }

            function onHeightChanged() {
                pickerPopup.vcReposition()
            }
        }

        padding: ( theme ? theme.spaceCompact : 8 )

        background: Rectangle {
            objectName: ( root.objectName && root.objectName.length > 0 )
                ? ( root.objectName + "PopupBackground" )
                : "vcFontPickerPopupBackground"
            radius: ( theme ? theme.panelRadius : 8 )
            color: ( theme ? theme.panelColour : pal.base )
            border.color: ( theme ? theme.frameColour : pal.mid )
            border.width: ( theme ? theme.lineMed : 1 )

            Rectangle {
                anchors.fill: parent
                anchors.margins: ( theme ? theme.insetGap : 3 )
                radius: Math.max( 0, ( theme ? theme.panelRadius : 8 ) - 2 )
                color: "transparent"
                border.color: ( theme ? theme.frameInnerColour : pal.midlight )
                border.width: 1
            }
        }

        contentItem: Item {
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: ( theme ? theme.spaceCompact : 8 )

            VcTextField {
                id: searchField
                objectName: ( root.objectName && root.objectName.length > 0 )
                    ? ( root.objectName + "SearchField" )
                    : "vcFontPickerSearchField"
                theme: root.theme
                Layout.fillWidth: true
                placeholderText: "Search fonts"
                font.family: Qt.application.font.family
            }

            Rectangle {
                objectName: ( root.objectName && root.objectName.length > 0 )
                    ? ( root.objectName + "ListFrame" )
                    : "vcFontPickerListFrame"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 80
                radius: ( theme ? theme.controlRadius : 10 )
                color: ( theme ? theme.panelInsetColour : pal.base )
                border.color: ( theme ? theme.frameColour : pal.mid )
                border.width: 1
                clip: true

                Item {
                    anchors.fill: parent
                    anchors.margins: 6

                    ListView {
                        id: fontList
                        objectName: ( root.objectName && root.objectName.length > 0 )
                            ? ( root.objectName + "List" )
                            : "vcFontPickerList"
                        anchors.fill: parent
                        clip: true
                        spacing: ( theme ? theme.spaceNudge : 4 )
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: VcScrollBar {
                            id: fontScrollBar
                            objectName: ( root.objectName && root.objectName.length > 0 )
                                ? ( root.objectName + "ScrollBar" )
                                : "vcFontPickerScrollBar"
                            theme: root.theme
                            policy: ScrollBar.AsNeeded
                            orientation: Qt.Vertical
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                        }

                        readonly property string q: searchField.text.trim().toLowerCase()

                        model: {
                            var out = [ "" ]
                            var seen = {}
                            for ( var i = 0; i < root.families.length; ++i ) {
                                var name = root.families[i]
                                if ( !name || name.length === 0 ) {
                                    continue
                                }
                                if ( seen[name] ) {
                                    continue
                                }
                                seen[name] = true
                                out.push( name )
                            }

                            // Filter.
                            if ( q.length > 0 ) {
                                var filtered = []
                                for ( var j = 0; j < out.length; ++j ) {
                                    var v = out[j]
                                    var label = ( v && v.length > 0 ) ? v : "System default"
                                    if ( label.toLowerCase().indexOf( q ) >= 0 ) {
                                        filtered.push( v )
                                    }
                                }
                                return filtered
                            }

                            return out
                        }

                        delegate: Rectangle {
                            id: row
                            width: fontList.width - ( fontScrollBar.visible ? ( fontScrollBar.width + 6 ) : 0 )
                            height: ( theme ? Math.max( theme.compactControlHeight + ( theme.spaceCompact * 2 ), 52 ) : 52 )
                            radius: ( theme ? theme.controlRadius : 8 )
                            color: ( ( modelData === root.value )
                                ? ( theme ? theme.tertiaryAccentColour : pal.highlight )
                                : "transparent" )
                            border.color: ( modelData === root.value )
                                ? ( theme ? theme.frameColour : pal.mid )
                                : "transparent"
                            border.width: ( modelData === root.value ) ? 1 : 0

                            readonly property string familyValue: modelData
                            readonly property string familyLabel: ( familyValue && familyValue.length > 0 ) ? familyValue : "System default"
                            readonly property int vcPopupSerial: root.vcPopupOpenSerial

                            readonly property int vcHealthStatus: {
                                // Include the serial in the dependency graph so this reevaluates when the cache changes.
                                var _ = root.vcHealthSerial
                                if ( !root.fontPreviewSafetyCache || !familyValue || familyValue.length === 0 ) {
                                    return 2 // Safe
                                }
                                return root.fontPreviewSafetyCache.statusForFamily( familyValue, root.vcPreviewPx )
                            }

                            function vcMaybeRequestCheck() {
                                if ( !root.fontPreviewSafetyCache ) {
                                    return
                                }
                                if ( !pickerPopup.opened ) {
                                    return
                                }
                                if ( !familyValue || familyValue.length === 0 ) {
                                    return
                                }
                                root.fontPreviewSafetyCache.requestCheck( familyValue, root.vcPreviewPx )
                            }

                            Component.onCompleted: vcMaybeRequestCheck()
                            onVisibleChanged: {
                                if ( visible ) {
                                    vcMaybeRequestCheck()
                                }
                            }

                            onVcPopupSerialChanged: vcMaybeRequestCheck()

                            Column {
                                anchors.fill: parent
                                anchors.margins: ( theme ? theme.spaceCompact : 8 )
                                spacing: 1

                                Text {
                                    text: row.familyLabel
                                    color: ( theme ? theme.textColour : pal.text )
                                    elide: Text.ElideRight
                                    width: parent.width
                                    clip: true
                                    font.family: root.vcFallbackFamily
                                    font.pixelSize: ( theme ? theme.fontBasePx : 14 )
                                }

                                Text {
                                    text: {
                                        if ( row.vcHealthStatus === 1 ) return "Checking preview..."
                                        if ( row.vcHealthStatus === 3 ) return "Preview unavailable: this font may not render correctly"
                                        if ( row.vcHealthStatus === 4 ) return "Preview unavailable right now"
                                        if ( row.vcHealthStatus === 5 ) return "Preview paused to keep the app responsive"
                                        return root.sampleText
                                    }
                                    color: ( row.vcHealthStatus === 3 || row.vcHealthStatus === 4 || row.vcHealthStatus === 5 )
                                        ? ( theme ? theme.dangerColour : pal.text )
                                        : ( theme ? theme.metaTextColour : pal.mid )
                                    elide: Text.ElideRight
                                    width: parent.width
                                    clip: true
                                    maximumLineCount: 1
                                    font.pixelSize: root.vcPreviewPx
                                    font.family: ( row.vcHealthStatus === 2 && row.familyValue && row.familyValue.length > 0 )
                                        ? row.familyValue
                                        : root.vcFallbackFamily
                                    font.bold: ( row.vcHealthStatus === 3 || row.vcHealthStatus === 4 || row.vcHealthStatus === 5 )
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    root.value = row.familyValue
                                    root.valueSelected( row.familyValue )
                                    pickerPopup.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
}
