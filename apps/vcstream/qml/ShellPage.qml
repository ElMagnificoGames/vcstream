import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtMultimedia 6.0

Page {
    id: root
    objectName: "shellPage"
    padding: 0

    property var uiMetrics
    property var appPalette
    property var theme

    SystemPalette {
        id: sysPal
    }

    readonly property var pal: ( appPalette ? appPalette : sysPal )

    readonly property color tintSoft: Qt.rgba( pal.text.r, pal.text.g, pal.text.b, 0.06 )
    readonly property color tintSofter: Qt.rgba( pal.text.r, pal.text.g, pal.text.b, 0.04 )
    readonly property color scrimColour: Qt.rgba( pal.shadow.r, pal.shadow.g, pal.shadow.b, 0.35 )
    readonly property color resolvedHighlightColour: palette.highlight
    readonly property color resolvedButtonColour: palette.button

    palette.window: ( theme ? theme.windowColour : pal.window )
    palette.windowText: ( theme ? theme.windowTextColour : pal.windowText )
    palette.base: ( theme ? theme.baseColour : pal.base )
    palette.alternateBase: ( theme ? theme.alternateBaseColour : pal.alternateBase )
    palette.text: ( theme ? theme.textColour : pal.text )
    palette.button: ( theme ? theme.buttonColour : pal.button )
    palette.buttonText: ( theme ? theme.buttonTextColour : pal.buttonText )
    palette.placeholderText: ( theme ? theme.placeholderTextColour : pal.placeholderText )
    palette.light: ( theme ? theme.lightColour : pal.light )
    palette.midlight: ( theme ? theme.midlightColour : pal.midlight )
    palette.dark: ( theme ? theme.darkColour : pal.dark )
    palette.mid: ( theme ? theme.midColour : pal.mid )
    palette.shadow: ( theme ? theme.shadowColour : pal.shadow )
    palette.highlight: ( theme ? theme.accentColour : pal.highlight )
    palette.highlightedText: ( theme ? theme.highlightedTextColour : pal.highlightedText )
    palette.link: ( theme ? theme.linkColour : pal.link )
    palette.linkVisited: ( theme ? theme.linkVisitedColour : pal.linkVisited )
    palette.toolTipBase: ( theme ? theme.toolTipBaseColour : pal.toolTipBase )
    palette.toolTipText: ( theme ? theme.toolTipTextColour : pal.toolTipText )

    background: Rectangle {
        color: ( theme ? theme.paperColour : pal.window )
    }

    Component.onCompleted: {
        scheduleRebuildSourcesModel()
    }

    signal disconnectRequested()
    signal preferencesRequested()
    signal schedulingRequested()
    signal supportRequested()

    property int selectedSourceIndex: -1
    property string selectedSourceKind: ""
    property string selectedSourceTitle: ""
    property string selectedSourceTitleLabel: ""
    property string selectedSourceDeviceId: ""
    property bool selectedSourceExportEnabled: false
    property bool selectedSourceIsAvailable: true
    property bool sourceInspectorOpen: false

    // Sources list scrollbars:
    // computed to avoid binding loops between ScrollBar visibility and ListView geometry.
    property bool sourcesNeedsHScroll: false
    property bool sourcesNeedsVScroll: false

    // Horizontal scroll support for long source labels.
    // This is a best-effort hint used to drive the ListView contentWidth.
    property real sourcesContentWidthHint: 0

    TextMetrics {
        id: sourcesHeaderMetrics
        font.family: ( theme ? theme.bodyFontFamily : Qt.application.font.family )
        font.pixelSize: ( theme ? Math.max( 12, theme.fontBasePx - 1 ) : 13 )
        font.capitalization: Font.SmallCaps
        font.letterSpacing: 1.2
    }

    TextMetrics {
        id: sourcesRowMetrics
        font.pixelSize: ( theme ? Math.max( 12, theme.fontBasePx - 1 ) : 13 )
        font.italic: false
    }

    TextMetrics {
        id: sourcesNoteMetrics
        font.pixelSize: ( theme ? Math.max( 12, theme.fontBasePx - 1 ) : 13 )
        font.italic: true
    }

    // Local preview state (Task 3.2).
    property string cameraPreviewDeviceId: ""
    property bool cameraPreviewActive: false
    property var cameraPreviewHandle: null

    readonly property bool cameraPreviewDisconnected: {
        if ( !root.cameraPreviewActive ) {
            return false
        }
        if ( root.cameraPreviewHandle === null ) {
            return false
        }
        if ( root.cameraPreviewHandle && root.cameraPreviewHandle.errorText && root.cameraPreviewHandle.errorText.length > 0 ) {
            return false
        }
        if ( !appSupervisor || !appSupervisor.deviceCatalogue ) {
            return false
        }
        return !root.isDeviceAvailable( appSupervisor.deviceCatalogue.cameras, root.cameraPreviewDeviceId )
    }

    function updateSelectedSourceTitle() {
        var prefix = ""
        if ( selectedSourceKind === "camera" ) {
            prefix = "Camera — "
        } else if ( selectedSourceKind === "microphone" ) {
            prefix = "Microphone — "
        } else if ( selectedSourceKind === "screen" ) {
            prefix = "Screen — "
        } else if ( selectedSourceKind === "window" ) {
            prefix = "Window — "
        } else if ( selectedSourceKind === "audioOutput" ) {
            prefix = "Audio output — "
        }

        var name = ( selectedSourceTitleLabel !== undefined ? selectedSourceTitleLabel : "" )
        if ( !selectedSourceIsAvailable && name.length > 0 ) {
            name = name + " (not available)"
        }

        selectedSourceTitle = prefix.length > 0 ? ( prefix + name ) : name
    }

    function openSourceInspectorDevice( kind, deviceId, titleLabel ) {
        var foundIndex = -1
        var foundExportEnabled = false
        var foundAvailable = true

        for ( var i = 0; i < sourceModel.count; ++i ) {
            var item = sourceModel.get( i )
            if ( item.rowType === "device" && item.kind === kind && item.deviceId === deviceId ) {
                foundIndex = i
                foundExportEnabled = item.exportEnabled
                foundAvailable = ( item.available !== false )
            }
        }

        selectedSourceIndex = foundIndex
        selectedSourceKind = kind
        selectedSourceDeviceId = deviceId
        selectedSourceExportEnabled = foundExportEnabled
        selectedSourceTitleLabel = ( titleLabel !== undefined ? titleLabel : "" )
        selectedSourceIsAvailable = foundAvailable

        if ( selectedSourceKind === "camera" ) {
            cameraPreviewDeviceId = selectedSourceDeviceId
            ensureCameraPreviewDeviceSelected()
        }

        updateSelectedSourceTitle()

        sourceInspectorOpen = true
    }

    Timer {
        id: sourcesRebuildTimer
        interval: 0
        repeat: false
        onTriggered: root.rebuildSourcesModel()
    }

    function scheduleRebuildSourcesModel() {
        if ( !sourcesRebuildTimer.running ) {
            sourcesRebuildTimer.start()
        }
    }

    function ensureSourcesHeaders() {
        if ( sourceModel.count > 0 ) {
            return
        }

        sourceModel.append( { rowType: "header", sectionKey: "cameras", label: "Cameras" } )
        sourceModel.append( { rowType: "header", sectionKey: "microphones", label: "Microphones" } )
        sourceModel.append( { rowType: "header", sectionKey: "screens", label: "Screens" } )
        sourceModel.append( { rowType: "header", sectionKey: "audioOutputs", label: "Audio outputs" } )
        sourceModel.append( { rowType: "header", sectionKey: "windows", label: "Windows" } )
    }

    function findSectionHeaderIndex( sectionKey ) {
        for ( var i = 0; i < sourceModel.count; ++i ) {
            var item = sourceModel.get( i )
            if ( item && item.rowType === "header" && item.sectionKey === sectionKey ) {
                return i
            }
        }
        return -1
    }

    function sectionRange( sectionKey ) {
        var headerIndex = findSectionHeaderIndex( sectionKey )
        if ( headerIndex < 0 ) {
            return { headerIndex: -1, start: -1, end: -1 }
        }

        var nextHeaderIndex = sourceModel.count
        for ( var i = headerIndex + 1; i < sourceModel.count; ++i ) {
            var item = sourceModel.get( i )
            if ( item && item.rowType === "header" ) {
                nextHeaderIndex = i
                break
            }
        }

        return { headerIndex: headerIndex, start: headerIndex + 1, end: nextHeaderIndex }
    }

    function normalisedRow( sectionKey, row ) {
        var out = {
            rowType: ( row.rowType !== undefined ? row.rowType : "" ),
            sectionKey: sectionKey,

            label: ( row.label !== undefined ? row.label : "" ),
            titleLabel: ( row.titleLabel !== undefined ? row.titleLabel : "" ),
            heightHint: ( row.heightHint !== undefined ? row.heightHint : 0 ),

            kind: ( row.kind !== undefined ? row.kind : "" ),
            deviceId: ( row.deviceId !== undefined ? row.deviceId : "" ),

            exportEnabled: ( row.exportEnabled !== undefined ? row.exportEnabled : false ),
            available: ( row.available !== undefined ? row.available : true )
        }
        return out
    }

    function setRowAt( index, row ) {
        sourceModel.setProperty( index, "rowType", row.rowType )
        sourceModel.setProperty( index, "sectionKey", row.sectionKey )
        sourceModel.setProperty( index, "label", row.label )
        sourceModel.setProperty( index, "titleLabel", row.titleLabel )
        sourceModel.setProperty( index, "heightHint", row.heightHint )
        sourceModel.setProperty( index, "kind", row.kind )
        sourceModel.setProperty( index, "deviceId", row.deviceId )
        sourceModel.setProperty( index, "exportEnabled", row.exportEnabled )
        sourceModel.setProperty( index, "available", row.available )
    }

    // Sources model update policy:
    // - Avoid clearing/rebuilding `sourceModel` or bulk-removing large ranges. Those operations cause
    //   large `contentHeight` discontinuities, and the ListView/Flickable then clamps `contentY`
    //   (visible scrollbar/thumb jump).
    // - Instead, reconcile each section in two passes:
    //   1) Replace-in-place existing rows and insert extra rows if the desired section grew.
    //   2) Remove trailing excess rows if the desired section shrank.
    // - Mental model: if a section should change from [a, b, c] to [b, c, d], first overwrite
    //   a->b, b->c, c->d (and insert if needed), then remove any leftovers.
    function reconcileSection( sectionKey, desiredRows ) {
        const range = sectionRange( sectionKey )
        if ( range.headerIndex < 0 ) {
            return
        }

        const start = range.start
        const end = range.end
        const existingCount = Math.max( 0, end - start )

        // First pass: replace existing rows, then add any extra rows.
        for ( var i = 0; i < desiredRows.length; ++i ) {
            const row = normalisedRow( sectionKey, desiredRows[i] )
            if ( i < existingCount ) {
                setRowAt( start + i, row )
            } else {
                sourceModel.insert( start + i, row )
            }
        }

        // Second pass: remove any excess rows.
        const after = sectionRange( sectionKey )
        const afterCount = Math.max( 0, after.end - after.start )
        if ( afterCount > desiredRows.length ) {
            sourceModel.remove( after.start + desiredRows.length, afterCount - desiredRows.length )
        }
    }

    function recomputeSourcesContentWidthHint() {
        var maxW = 0

        for ( var i = 0; i < sourceModel.count; ++i ) {
            var item = sourceModel.get( i )
            if ( !item || item.label === undefined ) {
                continue
            }

            if ( item.rowType === "header" ) {
                sourcesHeaderMetrics.text = item.label
                maxW = Math.max( maxW, sourcesHeaderMetrics.advanceWidth )
            } else if ( item.rowType === "note" ) {
                sourcesNoteMetrics.text = item.label
                maxW = Math.max( maxW, sourcesNoteMetrics.advanceWidth )
            } else {
                sourcesRowMetrics.text = item.label
                maxW = Math.max( maxW, sourcesRowMetrics.advanceWidth )
            }
        }

        // Delegate margins, bullet/indicator, and a little breathing room.
        const pad = 2 * ( theme ? theme.spaceCompact : 8 ) + 24
        root.sourcesContentWidthHint = Math.ceil( maxW + pad )
    }

    function recomputeSourcesScrollNeeds() {
        if ( !sourcesFrame || !sourcesList || !sourcesScrollBar || !sourcesHScrollBar ) {
            return
        }

        const leftPad = ( theme ? theme.spaceCompact : 8 )
        const rightPad = ( theme ? theme.spaceCompact : 8 )
        const topPad = ( theme ? theme.spaceNudge : 4 )
        const bottomPad = ( theme ? theme.spaceCompact : 8 )
        const gap = ( theme ? theme.spaceNudge : 4 )

        const vBarW = sourcesScrollBar.implicitWidth
        const hBarH = sourcesHScrollBar.implicitHeight

        const viewW0 = Math.max( 0, sourcesFrame.width - leftPad - rightPad )
        const viewH0 = Math.max( 0, sourcesFrame.height - topPad - bottomPad )

        var needH = ( root.sourcesContentWidthHint > viewW0 + 1 )
        var viewH1 = viewH0 - ( needH ? ( hBarH + gap ) : 0 )
        var needV = ( sourcesList.contentHeight > viewH1 + 1 )

        var viewW1 = viewW0 - ( needV ? ( vBarW + gap ) : 0 )
        needH = ( root.sourcesContentWidthHint > viewW1 + 1 )
        viewH1 = viewH0 - ( needH ? ( hBarH + gap ) : 0 )
        needV = ( sourcesList.contentHeight > viewH1 + 1 )

        root.sourcesNeedsHScroll = needH
        root.sourcesNeedsVScroll = needV
    }

    function resyncSelectedSourceIndexFromIdentity() {
        if ( !root.selectedSourceKind || root.selectedSourceKind.length <= 0 ) {
            root.selectedSourceIndex = -1
            root.selectedSourceExportEnabled = false
            return
        }
        if ( !root.selectedSourceDeviceId || root.selectedSourceDeviceId.length <= 0 ) {
            root.selectedSourceIndex = -1
            root.selectedSourceExportEnabled = false
            return
        }

        var foundIndex = -1
        var foundExportEnabled = false
        var foundAvailable = true
        var foundTitleLabel = ""

        for ( var i = 0; i < sourceModel.count; ++i ) {
            var item = sourceModel.get( i )
            if ( item.rowType === "device" && item.kind === root.selectedSourceKind && item.deviceId === root.selectedSourceDeviceId ) {
                foundIndex = i
                foundExportEnabled = item.exportEnabled
                foundAvailable = ( item.available !== false )
                foundTitleLabel = ( item.titleLabel !== undefined ? item.titleLabel : "" )
            }
        }

        root.selectedSourceIndex = foundIndex
        root.selectedSourceExportEnabled = foundExportEnabled

        if ( foundIndex >= 0 ) {
            root.selectedSourceIsAvailable = foundAvailable
            root.selectedSourceTitleLabel = foundTitleLabel
            root.updateSelectedSourceTitle()
        }
    }

    function rebuildSourcesModel() {
        ensureSourcesHeaders()

        var exportMap = {}
        for ( var i = 0; i < sourceModel.count; ++i ) {
            var existing = sourceModel.get( i )
            if ( existing.rowType === "device" && existing.deviceId !== undefined ) {
                exportMap[ existing.kind + ":" + existing.deviceId ] = existing.exportEnabled
            }
        }

        function makeDeviceRow( kind, deviceId, titleLabel, available ) {
            const key = kind + ":" + deviceId
            const enabled = ( exportMap[key] !== undefined ? exportMap[key] : false )
            const isAvailable = ( available !== false )
            var listLabel = titleLabel
            if ( !isAvailable && titleLabel.length > 0 ) {
                listLabel = titleLabel + " (not available)"
            }
            return {
                rowType: "device",
                kind: kind,
                deviceId: deviceId,
                titleLabel: titleLabel,
                label: "- " + listLabel,
                exportEnabled: enabled,
                available: isAvailable
            }
        }

        function makeNoteRow( text ) {
            return { rowType: "note", label: text }
        }

        function makeSpacerRow( heightHint ) {
            return { rowType: "spacer", heightHint: ( heightHint !== undefined ? heightHint : 0 ) }
        }

        const dc = ( appSupervisor ? appSupervisor.deviceCatalogue : null )

        var hasAvailableCamera = false
        if ( dc && dc.cameras ) {
            for ( var cAvail = 0; cAvail < dc.cameras.length; ++cAvail ) {
                if ( dc.cameras[cAvail].available !== false ) {
                    hasAvailableCamera = true
                }
            }
        }

        var cameraRows = []
        if ( dc && dc.cameras ) {
            for ( var c = 0; c < dc.cameras.length; ++c ) {
                if ( !dc.cameras[c] ) {
                    continue
                }
                const available = ( dc.cameras[c].available !== false )
                const titleLabel = dc.cameras[c].name
                cameraRows.push( makeDeviceRow( "camera", dc.cameras[c].id, titleLabel, available ) )
            }
        }

        if ( !hasAvailableCamera ) {
            if ( dc && dc.cameraDiscoveryStatus && dc.cameraDiscoveryStatus.length > 0 ) {
                cameraRows.push( makeNoteRow( dc.cameraDiscoveryStatus ) )
            } else {
                cameraRows.push( makeNoteRow( "No cameras detected." ) )
            }
            cameraRows.push( makeSpacerRow() )
        }

        reconcileSection( "cameras", cameraRows )

        var hasAvailableMic = false
        if ( dc && dc.microphones ) {
            for ( var mAvail = 0; mAvail < dc.microphones.length; ++mAvail ) {
                if ( dc.microphones[mAvail].available !== false ) {
                    hasAvailableMic = true
                }
            }
        }

        var microphoneRows = []
        if ( dc && dc.microphones ) {
            for ( var m = 0; m < dc.microphones.length; ++m ) {
                if ( !dc.microphones[m] ) {
                    continue
                }
                const available = ( dc.microphones[m].available !== false )
                const titleLabel = dc.microphones[m].name
                microphoneRows.push( makeDeviceRow( "microphone", dc.microphones[m].id, titleLabel, available ) )
            }
        }

        if ( !hasAvailableMic ) {
            microphoneRows.push( makeNoteRow( "No microphones detected." ) )
            microphoneRows.push( makeSpacerRow() )
        }

        reconcileSection( "microphones", microphoneRows )

        var screenRows = []
        if ( !dc || !dc.screens || dc.screens.length <= 0 ) {
            screenRows.push( makeNoteRow( "No screens detected." ) )
            screenRows.push( makeSpacerRow() )
        } else {
            for ( var s = 0; s < dc.screens.length; ++s ) {
                const screenLabel = dc.screens[s].name
                const screenId = ( dc.screens[s].id !== undefined ? dc.screens[s].id : ( "screen:" + s ) )
                screenRows.push( makeDeviceRow( "screen", screenId, screenLabel, true ) )
            }
        }

        reconcileSection( "screens", screenRows )

        var hasAvailableOut = false
        if ( dc && dc.audioOutputs ) {
            for ( var oAvail = 0; oAvail < dc.audioOutputs.length; ++oAvail ) {
                if ( dc.audioOutputs[oAvail].available !== false ) {
                    hasAvailableOut = true
                }
            }
        }

        var audioOutputRows = []
        if ( dc && dc.audioOutputs ) {
            for ( var o = 0; o < dc.audioOutputs.length; ++o ) {
                if ( !dc.audioOutputs[o] ) {
                    continue
                }
                const available = ( dc.audioOutputs[o].available !== false )
                const titleLabel = dc.audioOutputs[o].name
                audioOutputRows.push( makeDeviceRow( "audioOutput", dc.audioOutputs[o].id, titleLabel, available ) )
            }
        }

        if ( !hasAvailableOut ) {
            audioOutputRows.push( makeNoteRow( "No audio outputs detected." ) )
            audioOutputRows.push( makeSpacerRow() )
        }

        reconcileSection( "audioOutputs", audioOutputRows )

        var windowRows = []
        if ( !dc || !dc.windows || dc.windows.length <= 0 ) {
            if ( dc && dc.windowCaptureStatus && dc.windowCaptureStatus.length > 0 ) {
                windowRows.push( makeNoteRow( dc.windowCaptureStatus ) )
            } else {
                windowRows.push( makeNoteRow( "No capturable windows reported." ) )
            }
            windowRows.push( makeSpacerRow() )
        } else {
            for ( var w = 0; w < dc.windows.length; ++w ) {
                if ( !dc.windows[w] ) {
                    continue
                }
                const windowId = ( dc.windows[w].id !== undefined ? dc.windows[w].id : ( "window:" + w ) )
                windowRows.push( makeDeviceRow( "window", windowId, dc.windows[w].name, true ) )
            }
        }

        reconcileSection( "windows", windowRows )

        resyncSelectedSourceIndexFromIdentity()

        recomputeSourcesContentWidthHint()
        recomputeSourcesScrollNeeds()
    }

    function updateWindowsSection() {
        ensureSourcesHeaders()

        if ( findSectionHeaderIndex( "windows" ) < 0 ) {
            root.scheduleRebuildSourcesModel()
            return
        }

        var exportMap = {}
        for ( var i = 0; i < sourceModel.count; ++i ) {
            var existing = sourceModel.get( i )
            if ( existing.rowType === "device" && existing.deviceId !== undefined ) {
                exportMap[ existing.kind + ":" + existing.deviceId ] = existing.exportEnabled
            }
        }

        function makeDeviceRow( kind, deviceId, label ) {
            const key = kind + ":" + deviceId
            const enabled = ( exportMap[key] !== undefined ? exportMap[key] : false )
            return {
                rowType: "device",
                kind: kind,
                deviceId: deviceId,
                label: "- " + label,
                exportEnabled: enabled,
                available: true
            }
        }

        function makeNoteRow( text ) { return { rowType: "note", label: text } }
        function makeSpacerRow() { return { rowType: "spacer", heightHint: 0 } }

        const dc = ( appSupervisor ? appSupervisor.deviceCatalogue : null )

        var windowRows = []
        if ( !dc || !dc.windows || dc.windows.length <= 0 ) {
            if ( dc && dc.windowCaptureStatus && dc.windowCaptureStatus.length > 0 ) {
                windowRows.push( makeNoteRow( dc.windowCaptureStatus ) )
            } else {
                windowRows.push( makeNoteRow( "No capturable windows reported." ) )
            }
            windowRows.push( makeSpacerRow() )
        } else {
            for ( var w = 0; w < dc.windows.length; ++w ) {
                if ( !dc.windows[w] ) {
                    continue
                }
                const windowId = ( dc.windows[w].id !== undefined ? dc.windows[w].id : ( "window:" + w ) )
                windowRows.push( makeDeviceRow( "window", windowId, dc.windows[w].name ) )
            }
        }

        reconcileSection( "windows", windowRows )

        // Preserve existing behaviour: the window section is volatile enough
        // that we do not keep the inspector selection when it changes.
        if ( root.selectedSourceKind === "window" ) {
            root.selectedSourceIndex = -1
            root.selectedSourceDeviceId = ""
            root.selectedSourceTitle = ""
            root.selectedSourceExportEnabled = false
        } else {
            resyncSelectedSourceIndexFromIdentity()
        }

        recomputeSourcesContentWidthHint()
        recomputeSourcesScrollNeeds()
    }

    function isDeviceAvailable( list, id ) {
        if ( !list || !id || id.length <= 0 ) {
            return false
        }

        for ( var i = 0; i < list.length; ++i ) {
            if ( list[i].id === id ) {
                return ( list[i].available !== false )
            }
        }
        return false
    }

    function decoratedDeviceList( list ) {
        if ( !list ) {
            return []
        }

        var out = []
        for ( var i = 0; i < list.length; ++i ) {
            var item = list[i]
            var name = item.name
            if ( item.available === false ) {
                name += " (not available)"
            }

            out.push( {
                id: item.id,
                name: name,
                isDefault: item.isDefault,
                available: item.available
            } )
        }

        return out
    }

    function ensureCameraPreviewDeviceSelected() {
        if ( !appSupervisor || !appSupervisor.deviceCatalogue ) {
            return
        }

        const cameras = appSupervisor.deviceCatalogue.cameras
        if ( !cameras || cameras.length <= 0 ) {
            cameraPreviewDeviceId = ""
            return
        }

        var hasAvailable = false
        for ( var a = 0; a < cameras.length; ++a ) {
            if ( cameras[a].available !== false ) {
                hasAvailable = true
            }
        }

        var found = false
        for ( var i = 0; i < cameras.length; ++i ) {
            if ( cameras[i].id === cameraPreviewDeviceId ) {
                found = true
            }
        }

        if ( found ) {
            return
        }

        if ( cameraPreviewDeviceId.length <= 0 && hasAvailable ) {
            for ( var j = 0; j < cameras.length; ++j ) {
                if ( cameras[j].available !== false && cameras[j].isDefault ) {
                    cameraPreviewDeviceId = cameras[j].id
                    return
                }
            }

            for ( var k = 0; k < cameras.length; ++k ) {
                if ( cameras[k].available !== false ) {
                    cameraPreviewDeviceId = cameras[k].id
                    return
                }
            }
        }
    }

    function closeSourceInspector() {
        hideHelp()
        stopCameraPreview()
        sourceInspectorOpen = false
    }

    function startCameraPreview() {
        if ( !appSupervisor || !appSupervisor.mediaCapture ) {
            return
        }

        ensureCameraPreviewDeviceSelected()
        if ( cameraPreviewDeviceId.length <= 0 ) {
            return
        }

        if ( cameraPreviewHandle ) {
            cameraPreviewHandle.close()
            cameraPreviewHandle = null
        }

        cameraPreviewHandle = appSupervisor.mediaCapture.acquireCameraPreviewHandle( cameraPreviewDeviceId, sourceInspectorPanel )
        if ( cameraPreviewHandle ) {
            cameraPreviewHandle.setViewSink( previewVideo.videoSink )
            cameraPreviewHandle.running = true
            cameraPreviewActive = true
        }
    }

    function stopCameraPreview() {
        cameraPreviewActive = false
        if ( cameraPreviewHandle ) {
            cameraPreviewHandle.running = false
            cameraPreviewHandle.close()
            cameraPreviewHandle = null
        }
    }


    function setSelectedSourceExportEnabled( enabled ) {
        if ( selectedSourceIndex < 0 ) {
            return
        }

        selectedSourceExportEnabled = enabled
        sourceModel.setProperty( selectedSourceIndex, "exportEnabled", enabled )
    }

    Connections {
        target: ( appSupervisor ? appSupervisor.deviceCatalogue : null )

        function onScreensChanged() { root.scheduleRebuildSourcesModel() }
        function onCamerasChanged() { root.scheduleRebuildSourcesModel() }
        function onMicrophonesChanged() { root.scheduleRebuildSourcesModel() }
        function onAudioOutputsChanged() { root.scheduleRebuildSourcesModel() }
        function onWindowsChanged() { root.updateWindowsSection() }
        function onWindowCaptureStatusChanged() { root.updateWindowsSection() }
        function onCameraDiscoveryStatusChanged() { root.scheduleRebuildSourcesModel() }
    }

    Item {
        id: overlayLayer
        anchors.fill: parent
        z: 10000

        Item {
            id: sourceInspectorOverlay
            objectName: "sourceInspectorOverlay"
            anchors.fill: parent
            visible: root.sourceInspectorOpen
            z: 200

            Rectangle {
                anchors.fill: parent
                color: scrimColour

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.closeSourceInspector()
                    }
                }
            }

            VcPanel {
                id: sourceInspectorPanel
                objectName: "sourceInspectorPanel"
                anchors.fill: parent
                anchors.margins: ( theme ? theme.spaceTight : 10 )
                padding: ( theme ? theme.spaceTight : 10 )
                theme: root.theme
                accentRole: "secondary"
                inset: true

                // Swallow any clicks inside the panel so the scrim doesn't close.
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.AllButtons
                    onPressed: function( mouse ) {
                        mouse.accepted = true
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: ( theme ? theme.spaceTight : 12 )

                    Rectangle {
                        Layout.fillWidth: true
                        height: ( theme ? Math.max( theme.compactControlHeight + ( theme.spaceCompact * 2 ), 52 ) : 52 )
                        radius: ( theme ? theme.panelRadius : 8 )
                        color: ( theme ? theme.panelColour : pal.base )
                        border.color: ( theme ? theme.frameColour : pal.mid )
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: ( theme ? theme.spaceCompact : 10 )
                            spacing: ( theme ? theme.spaceTight : 10 )

                                Label {
                                    objectName: "sourceInspectorTitleLabel"
                                    text: ( root.selectedSourceTitle.length > 0 ? root.selectedSourceTitle : "Source" )
                                    font.pixelSize: ( theme ? theme.fontHeadingPx : 18 )
                                    font.letterSpacing: 0.8
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    color: ( theme ? theme.textColour : pal.text )
                                }

                            VcCloseButton {
                                id: closeInspectorButton
                                objectName: "sourceInspectorCloseButton"
                                theme: root.theme
                                onClicked: {
                                    root.closeSourceInspector()
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: ( theme ? theme.panelInsetColour : tintSoft )

                        // Camera preview (only) for now.
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: ( theme ? theme.spaceCompact : 8 )
                            spacing: ( theme ? theme.spaceCompact : 8 )
                            visible: ( root.selectedSourceKind === "camera" )

                            Timer {
                                id: cameraPreviewRestartTimer
                                interval: 0
                                repeat: false
                                onTriggered: {
                                    root.startCameraPreview()
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: ( theme ? theme.spaceCompact : 8 )

                                Label {
                                    text: "Camera"
                                    color: ( theme ? theme.textColour : pal.text )
                                }

                                VcComboBox {
                                    id: cameraPreviewCombo
                                    objectName: "cameraPreviewCombo"
                                    theme: root.theme
                                    Layout.fillWidth: true
                                    enabled: ( appSupervisor && appSupervisor.deviceCatalogue && appSupervisor.deviceCatalogue.cameras.length > 0 )
                                    model: ( appSupervisor && appSupervisor.deviceCatalogue
                                        ? root.decoratedDeviceList( appSupervisor.deviceCatalogue.cameras )
                                        : [] )
                                    textRole: "name"

                                    function indexForDeviceId( id ) {
                                        if ( !model || model.length <= 0 ) {
                                            return -1
                                        }
                                        for ( var i = 0; i < model.length; ++i ) {
                                            if ( model[i].id === id ) {
                                                return i
                                            }
                                        }
                                        return -1
                                    }

                                    currentIndex: enabled ? indexForDeviceId( root.cameraPreviewDeviceId ) : -1

                                    onActivated: function( index ) {
                                        if ( index >= 0 && index < model.length ) {
                                            root.cameraPreviewDeviceId = model[index].id
                                        }

                                        if ( root.cameraPreviewActive ) {
                                            root.stopCameraPreview()
                                            cameraPreviewRestartTimer.restart()
                                        }
                                    }
                                }

                                VcButton {
                                    id: cameraPreviewToggleButton
                                    objectName: "cameraPreviewToggleButton"
                                    theme: root.theme
                                    tone: ( root.cameraPreviewActive ? "secondary" : "primary" )
                                    text: ( root.cameraPreviewActive ? "Stop preview" : "Start preview" )
                                    enabled: ( root.cameraPreviewActive
                                        || ( appSupervisor && appSupervisor.deviceCatalogue
                                            && root.isDeviceAvailable( appSupervisor.deviceCatalogue.cameras, root.cameraPreviewDeviceId ) ) )
                                    hoverEnabled: true
                                    property bool testSkipActivate: true

                                    onHoveredChanged: {
                                        if ( hovered ) {
                                            root.showHelp( cameraPreviewToggleButton, "Show your local camera feed in the preview panel." )
                                        } else {
                                            root.hideHelp()
                                        }
                                    }

                                    onClicked: {
                                        if ( root.cameraPreviewActive ) {
                                            root.stopCameraPreview()
                                        } else {
                                            root.startCameraPreview()
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                id: cameraPreviewFrame
                                objectName: "cameraPreviewFrame"
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 8
                                color: Qt.rgba( pal.shadow.r, pal.shadow.g, pal.shadow.b, 0.08 )
                                border.color: ( theme ? theme.frameInnerColour : pal.midlight )
                                border.width: 1

                                VideoOutput {
                                    id: previewVideo
                                    objectName: "cameraPreviewVideoOutput"
                                    anchors.fill: parent
                                    fillMode: VideoOutput.PreserveAspectFit
                                    visible: ( root.cameraPreviewHandle !== null
                                        && !( root.cameraPreviewHandle.errorText && root.cameraPreviewHandle.errorText.length > 0 ) )
                                    opacity: ( root.cameraPreviewDisconnected ? 0.35 : 1.0 )
                                }

                                Rectangle {
                                    objectName: "cameraPreviewDisconnectedOverlay"
                                    anchors.fill: parent
                                    radius: cameraPreviewFrame.radius
                                    color: scrimColour
                                    visible: root.cameraPreviewDisconnected

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        width: Math.min( parent.width - 16, 260 )
                                        spacing: 6

                                        Label {
                                            Layout.fillWidth: true
                                            horizontalAlignment: Text.AlignHCenter
                                            text: "Disconnected"
                                            font.pixelSize: ( theme ? theme.fontHeadingPx : 18 )
                                            color: ( theme ? theme.windowTextColour : pal.windowText )
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            wrapMode: Text.Wrap
                                            horizontalAlignment: Text.AlignHCenter
                                            text: "This camera is not available right now. Turn it on, or choose another camera."
                                            font.pixelSize: ( theme ? theme.fontBasePx : 14 )
                                            color: ( theme ? theme.windowTextColour : pal.windowText )
                                        }
                                    }
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: {
                                        if ( !root.cameraPreviewActive ) {
                                            return "Preview is stopped."
                                        }
                                        if ( root.cameraPreviewHandle && root.cameraPreviewHandle.errorText && root.cameraPreviewHandle.errorText.length > 0 ) {
                                            return root.cameraPreviewHandle.errorText
                                        }
                                        if ( !appSupervisor || !appSupervisor.deviceCatalogue || !appSupervisor.deviceCatalogue.cameras ) {
                                            return "No camera devices detected."
                                        }

                                        var hasAvailable = false
                                        for ( var i = 0; i < appSupervisor.deviceCatalogue.cameras.length; ++i ) {
                                            if ( appSupervisor.deviceCatalogue.cameras[i].available !== false ) {
                                                hasAvailable = true
                                            }
                                        }
                                        if ( !hasAvailable ) {
                                            if ( appSupervisor.deviceCatalogue.cameraDiscoveryStatus && appSupervisor.deviceCatalogue.cameraDiscoveryStatus.length > 0 ) {
                                                return appSupervisor.deviceCatalogue.cameraDiscoveryStatus
                                            }
                                            return "No camera devices detected."
                                        }
                                        return "Starting camera..."
                                    }
                                    visible: ( root.cameraPreviewHandle === null
                                        || ( root.cameraPreviewHandle && root.cameraPreviewHandle.errorText && root.cameraPreviewHandle.errorText.length > 0 ) )
                                    color: ( theme ? theme.metaTextColour : pal.mid )
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            text: "Preview (placeholder)"
                            opacity: 0.75
                            visible: ( root.selectedSourceKind !== "camera" )
                        }
                    }

                    VcPanel {
                        id: sourceInspectorDetailsPanel
                        objectName: "sourceInspectorDetailsPanel"
                        Layout.fillWidth: true
                        theme: root.theme
                        accentRole: "tertiary"
                        visible: ( root.selectedSourceKind === "camera" )

                        ColumnLayout {
                            id: cameraDetailsLayout
                            width: parent.width
                            spacing: 6

                            Label {
                                text: "Details"
                                font.pixelSize: ( theme ? theme.fontBasePx : 14 )
                                color: ( theme ? theme.textColour : pal.text )
                            }

                            readonly property string vcCameraOriginText: {
                                if ( !appSupervisor || !appSupervisor.deviceCatalogue || !appSupervisor.deviceCatalogue.cameras ) {
                                    return "Origin: (unknown camera)"
                                }
                                const cameras = appSupervisor.deviceCatalogue.cameras
                                if ( cameras.length <= 0 ) {
                                    return "Origin: (no camera detected)"
                                }
                                for ( var i = 0; i < cameras.length; ++i ) {
                                    if ( cameras[i].id === root.cameraPreviewDeviceId ) {
                                        return "Origin: " + cameras[i].name
                                    }
                                }
                                return "Origin: (unknown camera)"
                            }

                            Label {
                                text: cameraDetailsLayout.vcCameraOriginText
                                wrapMode: Text.Wrap
                                color: ( theme ? theme.metaTextColour : pal.mid )
                            }

                            Label {
                                text: "Owner: This device"
                                wrapMode: Text.Wrap
                                color: ( theme ? theme.metaTextColour : pal.mid )
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        VcCheckBox {
                            id: inspectorExportToggle
                            objectName: "sourceInspectorBrowserExportToggle"
                            theme: root.theme
                            text: "Browser Export"
                            hoverEnabled: true
                            checked: root.selectedSourceExportEnabled

                            onHoveredChanged: {
                                if ( hovered ) {
                                    root.showHelp( inspectorExportToggle, "Expose this source via a local URL for browser-source tools." )
                                } else {
                                    root.hideHelp()
                                }
                            }

                            onToggled: {
                                root.setSelectedSourceExportEnabled( checked )
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }

    HelpTip {
        id: hoverHelp
        overlay: overlayLayer
        appPalette: root.appPalette
    }


    Timer {
        id: hoverHelpTimer
        interval: 250
        repeat: false

        onTriggered: {
            hoverHelp.show()
        }
    }

    function showHelp( target, text ) {
        hoverHelp.target = target
        hoverHelp.text = text
        hoverHelpTimer.restart()
    }

    function hideHelp() {
        hoverHelpTimer.stop()
        hoverHelp.hide()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            contentHeight: ( theme ? theme.toolbarHeight : 56 )

            background: Rectangle {
                color: ( theme ? theme.panelColour : pal.base )
                border.color: ( theme ? theme.frameColour : pal.mid )
                border.width: ( theme ? theme.lineMed : 1 )

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: ( theme ? theme.insetGap : 3 )
                    color: "transparent"
                    border.color: ( theme ? theme.frameInnerColour : pal.midlight )
                    border.width: 1
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: ( theme ? theme.spaceTight : 10 )
                anchors.rightMargin: ( theme ? theme.spaceTight : 10 )
                anchors.topMargin: ( theme ? theme.spaceNudge : 6 )
                anchors.bottomMargin: ( theme ? theme.spaceNudge : 6 )
                spacing: ( theme ? theme.spaceTight : 12 )

                Label {
                    text: "VCStream"
                    font.pixelSize: ( theme ? theme.fontShellTitlePx : 22 )
                    font.family: ( theme ? theme.headingFontFamily : Qt.application.font.family )
                    font.capitalization: Font.SmallCaps
                    color: ( theme ? theme.textColour : pal.text )
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                VcButton {
                    id: disconnectButton
                    objectName: "disconnectButton"
                    theme: root.theme
                    tone: "danger"
                    text: "Disconnect"
                    hoverEnabled: true

                    onHoveredChanged: {
                        if ( hovered ) {
                            root.showHelp( disconnectButton, "Leave the room and return to the start screen." )
                        } else {
                            root.hideHelp()
                        }
                    }

                    onClicked: {
                        root.hideHelp()
                        root.disconnectRequested()
                    }
                }

                VcUtilityActions {
                    id: shellUtilityActions
                    theme: root.theme
                    schedulingObjectName: "shellSchedulingButton"
                    supportObjectName: "shellSupportButton"
                    preferencesObjectName: "preferencesButton"

                    onSchedulingRequested: {
                        root.hideHelp()
                        root.schedulingRequested()
                    }
                    onSupportRequested: {
                        root.hideHelp()
                        root.supportRequested()
                    }
                    onPreferencesRequested: {
                        root.hideHelp()
                        root.preferencesRequested()
                    }
                    onHoverHelpRequested: function( target, text ) { root.showHelp( target, text ) }
                    onHoverHelpHideRequested: root.hideHelp()
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            handle: Item {
                implicitWidth: ( theme ? theme.spaceNudge : 6 )

                Rectangle {
                    anchors.centerIn: parent
                    width: 1
                    height: parent.height
                    color: "transparent"
                }
            }

            Item {
                SplitView.preferredWidth: 260
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellLeftPaneMinWidth : 220 )
                SplitView.fillWidth: false

                Item {
                    anchors.fill: parent
                    anchors.margins: ( theme ? theme.spaceTight : 10 )

                    SplitView {
                        anchors.fill: parent
                        orientation: Qt.Vertical

                        handle: Item {
                            implicitHeight: ( theme ? theme.spaceTight : 12 )

                            Rectangle {
                                anchors.centerIn: parent
                                width: 1
                                height: parent.height
                                color: "transparent"
                            }
                        }

                        Item {
                            SplitView.fillWidth: true
                            SplitView.fillHeight: true
                            SplitView.minimumHeight: ( theme && theme.macroPx ? theme.macroPx( 160 ) : 160 )

                            VcPanel {
                                anchors.fill: parent
                                theme: root.theme
                                accentRole: "secondary"

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 8

                                    Label {
                                        text: "Participants"
                                        font.pixelSize: ( theme ? theme.fontBasePx : 14 )
                                        color: ( theme ? theme.textColour : pal.text )
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: 6
                                        color: ( theme ? theme.panelInsetColour : tintSoft )

                                        Label {
                                            anchors.centerIn: parent
                                            text: "(placeholder)"
                                            color: ( theme ? theme.metaTextColour : pal.mid )
                                        }
                                    }
                                }
                            }
                        }

                        Item {
                            SplitView.fillWidth: true
                            SplitView.preferredHeight: ( theme && theme.macroPx ? theme.macroPx( 240 ) : 240 )
                            SplitView.minimumHeight: ( theme && theme.macroPx ? theme.macroPx( 180 ) : 180 )

                            VcPanel {
                                anchors.fill: parent
                                theme: root.theme
                                accentRole: "tertiary"

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 8

                                    Label {
                                        text: "Sources"
                                        font.pixelSize: ( theme ? theme.fontBasePx : 14 )
                                        color: ( theme ? theme.textColour : pal.text )
                                    }

                                    ListModel {
                                        id: sourceModel
                                    }

                                    Rectangle {
                                        id: sourcesFrame
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: 6
                                        color: ( theme ? theme.panelInsetColour : tintSoft )

                                        onWidthChanged: sourcesScrollNeedsTimer.restart()
                                        onHeightChanged: sourcesScrollNeedsTimer.restart()

                                        Timer {
                                            id: sourcesScrollNeedsTimer
                                            interval: 0
                                            repeat: false
                                            onTriggered: root.recomputeSourcesScrollNeeds()
                                        }

                                        ListView {
                                            id: sourcesList
                                            objectName: "sourcesList"
                                            anchors.fill: parent
                                            anchors.leftMargin: ( theme ? theme.spaceCompact : 8 )
                                            anchors.rightMargin: ( theme ? theme.spaceCompact : 8 )
                                                + ( root.sourcesNeedsVScroll
                                                    ? ( sourcesScrollBar.implicitWidth + ( theme ? theme.spaceNudge : 4 ) )
                                                    : 0 )
                                            anchors.bottomMargin: ( theme ? theme.spaceCompact : 8 )
                                                + ( root.sourcesNeedsHScroll
                                                    ? ( sourcesHScrollBar.implicitHeight + ( theme ? theme.spaceNudge : 4 ) )
                                                    : 0 )
                                            anchors.topMargin: ( theme ? theme.spaceNudge : 4 )
                                            clip: true
                                            model: sourceModel
                                            spacing: 6
                                            flickableDirection: Flickable.HorizontalAndVerticalFlick

                                            contentWidth: Math.max( width, root.sourcesContentWidthHint )

                                            ScrollBar.vertical: sourcesScrollBar
                                            ScrollBar.horizontal: sourcesHScrollBar

                                            delegate: Rectangle {
                                                width: {
                                                    const vBarSpace = root.sourcesNeedsVScroll
                                                        ? ( sourcesScrollBar.implicitWidth + ( theme ? theme.spaceNudge : 4 ) )
                                                        : 0
                                                    return Math.max( ListView.view.width - vBarSpace, root.sourcesContentWidthHint - vBarSpace )
                                                }
                                                height: {
                                                    if ( rowType === "header" ) {
                                                        return ( theme ? Math.max( 22, theme.fontBasePx + 6 ) : 24 )
                                                    }
                                                    if ( rowType === "note" ) {
                                                        return ( theme ? Math.max( 20, theme.fontBasePx + 4 ) : 22 )
                                                    }
                                                    if ( rowType === "spacer" ) {
                                                        if ( heightHint !== undefined && heightHint > 0 ) {
                                                            return heightHint
                                                        }
                                                        return ( theme ? theme.spaceCompact : 8 )
                                                    }
                                                    return ( theme ? theme.compactControlHeight : 34 )
                                                }
                                                radius: ( rowType === "device" ? 6 : 0 )
                                                color: ( rowType === "device" ? tintSofter : "transparent" )

                                                objectName: {
                                                    if ( rowType === "header" ) {
                                                        return "sourceHeader_" + sectionKey
                                                    }
                                                    if ( rowType === "note" ) {
                                                        return "sourceNote_" + sectionKey
                                                    }
                                                    if ( rowType === "spacer" ) {
                                                        return "sourceSpacer_" + sectionKey
                                                    }
                                                    return "sourceRow_" + kind + "_" + deviceId
                                                }

                                                ColumnLayout {
                                                    anchors.fill: parent
                                                    anchors.margins: ( theme ? theme.spaceNudge : 6 )
                                                    spacing: 3
                                                    visible: ( rowType === "header" )

                                                    RowLayout {
                                                        Layout.fillWidth: true

                                                        Label {
                                                            text: label
                                                            Layout.fillWidth: true
                                                            elide: Text.ElideNone
                                                            wrapMode: Text.NoWrap
                                                            clip: true
                                                            color: ( theme ? theme.metaTextColour : pal.mid )
                                                            font.family: ( theme ? theme.bodyFontFamily : Qt.application.font.family )
                                                            font.pixelSize: ( theme ? Math.max( 12, theme.fontBasePx - 1 ) : 13 )
                                                            font.capitalization: Font.SmallCaps
                                                            font.letterSpacing: 1.2
                                                        }
                                                    }

                                                    Rectangle {
                                                        Layout.fillWidth: true
                                                        Layout.preferredHeight: 1
                                                        color: ( theme ? theme.frameInnerColour : pal.midlight )
                                                        opacity: 0.85
                                                    }
                                                }

                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.margins: ( theme ? theme.spaceCompact : 8 )
                                                    spacing: ( theme ? theme.spaceTight : 10 )
                                                    visible: ( rowType === "device" || rowType === "note" )

                                                    Label {
                                                        text: label
                                                        Layout.fillWidth: true
                                                        elide: Text.ElideNone
                                                        wrapMode: Text.NoWrap
                                                        clip: true
                                                        color: ( theme ? ( rowType === "note" ? theme.metaTextColour : theme.textColour )
                                                            : ( rowType === "note" ? pal.mid : pal.text ) )
                                                        font.pixelSize: ( theme ? Math.max( 12, theme.fontBasePx - 1 ) : 13 )
                                                        font.italic: ( rowType === "note" )
                                                    }

                                                    Rectangle {
                                                        Layout.preferredWidth: 10
                                                        Layout.preferredHeight: 10
                                                        radius: 5
                                                        visible: ( rowType === "device" ) && exportEnabled
                                                        color: pal.highlight
                                                        opacity: 0.9
                                                    }
                                                }

                                                MouseArea {
                                                    anchors.fill: parent
                                                    enabled: ( rowType === "device" )
                                                    onClicked: {
            root.openSourceInspectorDevice( kind, deviceId, titleLabel )
        }
                                                }
                                            }
                                        }

                                        VcScrollBar {
                                            id: sourcesScrollBar
                                            objectName: "sourcesScrollBar"
                                            theme: root.theme
                                            policy: ScrollBar.AsNeeded
                                            orientation: Qt.Vertical

                                            readonly property real vcGap: ( theme ? theme.spaceNudge : 4 )

                                            x: sourcesFrame.width - ( theme ? theme.spaceCompact : 8 ) - width
                                            y: ( theme ? theme.spaceNudge : 4 )
                                            height: sourcesFrame.height
                                                - ( theme ? theme.spaceNudge : 4 )
                                                - ( theme ? theme.spaceCompact : 8 )
                                                - ( root.sourcesNeedsHScroll ? ( sourcesHScrollBar.implicitHeight + vcGap ) : 0 )
                                        }

                                        VcScrollBar {
                                            id: sourcesHScrollBar
                                            objectName: "sourcesHScrollBar"
                                            theme: root.theme
                                            policy: ScrollBar.AsNeeded
                                            orientation: Qt.Horizontal

                                            readonly property real vcGap: ( theme ? theme.spaceNudge : 4 )

                                            x: ( theme ? theme.spaceCompact : 8 )
                                            y: sourcesFrame.height - ( theme ? theme.spaceCompact : 8 ) - height
                                            width: sourcesFrame.width
                                                - ( theme ? theme.spaceCompact : 8 )
                                                - ( theme ? theme.spaceCompact : 8 )
                                                - ( root.sourcesNeedsVScroll ? ( sourcesScrollBar.implicitWidth + vcGap ) : 0 )
                                        }
                                    }

                                    Label {
                                        text: "Browser Export is placeholder-only until the HTTP server exists."
                                        color: ( theme ? theme.metaTextColour : pal.mid )
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                SplitView.preferredWidth: 360
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellCentrePaneMinWidth : 240 )
                SplitView.fillWidth: true

                VcPanel {
                    anchors.fill: parent
                    anchors.margins: ( theme ? theme.spaceTight : 10 )
                    theme: root.theme
                    accentRole: "primary"

                    Item {
                        id: stageRoot
                        objectName: "stageRoot"
                        anchors.fill: parent

                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: ( theme ? theme.panelInsetColour : tintSofter )

                            Label {
                                anchors.centerIn: parent
                                text: "Stage / Participant Grid"
                                font.pixelSize: ( theme ? theme.fontHeadingPx : 18 )
                                color: ( theme ? theme.metaTextColour : pal.mid )
                            }
                        }

                    }
                }
            }

            Item {
                SplitView.preferredWidth: 320
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellRightPaneMinWidth : 260 )
                SplitView.fillWidth: false

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ( theme ? theme.spaceTight : 10 )
                    spacing: ( theme ? theme.spaceTight : 10 )

                    VcTabBar {
                        id: rightTabs
                        Layout.fillWidth: true
                        theme: root.theme

                        VcTabButton { objectName: "rightTabChat"; text: "Chat"; theme: root.theme }
                        VcTabButton { objectName: "rightTabDiagnostics"; text: "Diagnostics"; theme: root.theme }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: rightTabs.currentIndex

                        VcPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            theme: root.theme
                            accentRole: "secondary"

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                Label {
                                    text: "Chat"
                                    font.pixelSize: ( theme ? theme.fontBasePx : 14 )
                                    color: ( theme ? theme.textColour : pal.text )
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6
                                    color: ( theme ? theme.panelInsetColour : tintSoft )

                                    Label {
                                        anchors.centerIn: parent
                                        text: "(placeholder)"
                                        color: ( theme ? theme.metaTextColour : pal.mid )
                                    }
                                }

                                VcTextField {
                                    Layout.fillWidth: true
                                    theme: root.theme
                                    placeholderText: "Type a message (placeholder)"
                                    enabled: false
                                }
                            }
                        }

                        VcPanel {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            theme: root.theme
                            accentRole: "tertiary"

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                Label {
                                    text: "Diagnostics"
                                    font.pixelSize: ( theme ? theme.fontBasePx : 14 )
                                    color: ( theme ? theme.textColour : pal.text )
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6
                                    color: ( theme ? theme.panelInsetColour : tintSoft )

                                    Label {
                                        anchors.centerIn: parent
                                        text: "(placeholder)"
                                        color: ( theme ? theme.metaTextColour : pal.mid )
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        ToolBar {
            Layout.fillWidth: true
            contentHeight: ( theme ? theme.statusBarHeight : 30 )

            background: Rectangle {
                color: ( theme ? theme.panelColour : pal.base )
                border.color: ( theme ? theme.frameColour : pal.mid )
                border.width: 1
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: ( theme ? theme.spaceTight : 10 )
                spacing: ( theme ? theme.spaceTight : 10 )

                Label {
                    text: ( appSupervisor ? ( "Version " + appSupervisor.appVersion ) : "" )
                    color: ( theme ? theme.metaTextColour : pal.mid )
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: {
                        if ( !appSupervisor ) {
                            return ""
                        }

                        var joinText = appSupervisor.joinRoomEnabled ? "join:on" : "join:off"
                        var hostText = appSupervisor.hostRoomEnabled ? "host:on" : "host:off"
                        return "Roles: " + joinText + "  " + hostText
                    }
                    color: ( theme ? theme.metaTextColour : pal.mid )
                }
            }
        }
    }
}
