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
        if ( appSupervisor && appSupervisor.deviceCatalogue ) {
            appSupervisor.deviceCatalogue.refresh()
        }
        rebuildSourcesModel()
    }

    signal disconnectRequested()
    signal preferencesRequested()
    signal schedulingRequested()
    signal supportRequested()

    property int selectedSourceIndex: -1
    property string selectedSourceKind: ""
    property string selectedSourceTitle: ""
    property string selectedSourceDeviceId: ""
    property bool selectedSourceExportEnabled: false
    property bool sourceInspectorOpen: false

    // Local preview state (Task 3.2).
    property string cameraPreviewDeviceId: ""
    property bool cameraPreviewActive: false
    property var cameraPreviewHandle: null

    function openSourceInspectorDevice( kind, deviceId, label ) {
        var foundIndex = -1
        var foundExportEnabled = false

        for ( var i = 0; i < sourceModel.count; ++i ) {
            var item = sourceModel.get( i )
            if ( item.rowType === "device" && item.kind === kind && item.deviceId === deviceId ) {
                foundIndex = i
                foundExportEnabled = item.exportEnabled
            }
        }

        selectedSourceIndex = foundIndex
        selectedSourceKind = kind
        selectedSourceDeviceId = deviceId
        selectedSourceExportEnabled = foundExportEnabled

        if ( selectedSourceKind === "camera" ) {
            cameraPreviewDeviceId = selectedSourceDeviceId
            ensureCameraPreviewDeviceSelected()
            selectedSourceTitle = "Camera — " + label
        } else if ( selectedSourceKind === "microphone" ) {
            selectedSourceTitle = "Microphone — " + label
        } else if ( selectedSourceKind === "screen" ) {
            selectedSourceTitle = "Screen — " + label
        } else if ( selectedSourceKind === "window" ) {
            selectedSourceTitle = "Window — " + label
        } else if ( selectedSourceKind === "audioOutput" ) {
            selectedSourceTitle = "Audio output — " + label
        } else {
            selectedSourceTitle = label
        }

        sourceInspectorOpen = true
    }

    function rebuildSourcesModel() {
        var exportMap = {}
        for ( var i = 0; i < sourceModel.count; ++i ) {
            var existing = sourceModel.get( i )
            if ( existing.rowType === "device" && existing.deviceId !== undefined ) {
                exportMap[ existing.kind + ":" + existing.deviceId ] = existing.exportEnabled
            }
        }

        sourceModel.clear()

        function appendHeader( sectionKey, title ) {
            sourceModel.append( {
                rowType: "header",
                sectionKey: sectionKey,
                label: title
            } )
        }

        function appendNote( sectionKey, text ) {
            sourceModel.append( {
                rowType: "note",
                sectionKey: sectionKey,
                label: text
            } )
        }

        function appendDevice( kind, deviceId, label ) {
            const key = kind + ":" + deviceId
            const enabled = ( exportMap[key] !== undefined ? exportMap[key] : false )
            sourceModel.append( {
                rowType: "device",
                kind: kind,
                deviceId: deviceId,
                label: label,
                exportEnabled: enabled
            } )
        }

        const dc = ( appSupervisor ? appSupervisor.deviceCatalogue : null )

        appendHeader( "cameras", "Cameras" )
        if ( !dc || !dc.cameras || dc.cameras.length <= 0 ) {
            appendDevice( "camera", "", "Camera preview" )
            appendNote( "cameras", "No cameras detected." )
        } else {
            for ( var c = 0; c < dc.cameras.length; ++c ) {
                appendDevice( "camera", dc.cameras[c].id, dc.cameras[c].name )
            }
        }

        appendHeader( "microphones", "Microphones" )
        if ( !dc || !dc.microphones || dc.microphones.length <= 0 ) {
            appendNote( "microphones", "No microphones detected." )
        } else {
            for ( var m = 0; m < dc.microphones.length; ++m ) {
                appendDevice( "microphone", dc.microphones[m].id, dc.microphones[m].name )
            }
        }

        appendHeader( "screens", "Screens" )
        if ( !dc || !dc.screens || dc.screens.length <= 0 ) {
            appendNote( "screens", "No screens detected." )
        } else {
            for ( var s = 0; s < dc.screens.length; ++s ) {
                const screenLabel = dc.screens[s].name
                appendDevice( "screen", "screen:" + s, screenLabel )
            }
        }

        appendHeader( "audioOutputs", "Audio outputs" )
        if ( !dc || !dc.audioOutputs || dc.audioOutputs.length <= 0 ) {
            appendNote( "audioOutputs", "No audio outputs detected." )
        } else {
            for ( var o = 0; o < dc.audioOutputs.length; ++o ) {
                appendDevice( "audioOutput", dc.audioOutputs[o].id, dc.audioOutputs[o].name )
            }
        }

        appendHeader( "windows", "Windows" )
        if ( !dc || !dc.windows || dc.windows.length <= 0 ) {
            if ( dc && dc.windowCaptureStatus && dc.windowCaptureStatus.length > 0 ) {
                appendNote( "windows", dc.windowCaptureStatus )
            } else {
                appendNote( "windows", "No capturable windows reported." )
            }
        } else {
            for ( var w = 0; w < dc.windows.length; ++w ) {
                appendDevice( "window", "window:" + w, dc.windows[w].name )
            }
        }
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

        var found = false
        for ( var i = 0; i < cameras.length; ++i ) {
            if ( cameras[i].id === cameraPreviewDeviceId ) {
                found = true
            }
        }

        if ( found ) {
            return
        }

        for ( var j = 0; j < cameras.length; ++j ) {
            if ( cameras[j].isDefault ) {
                cameraPreviewDeviceId = cameras[j].id
                return
            }
        }

        cameraPreviewDeviceId = cameras[0].id
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

        function onScreensChanged() { root.rebuildSourcesModel() }
        function onCamerasChanged() { root.rebuildSourcesModel() }
        function onMicrophonesChanged() { root.rebuildSourcesModel() }
        function onAudioOutputsChanged() { root.rebuildSourcesModel() }
        function onWindowsChanged() { root.rebuildSourcesModel() }
        function onWindowCaptureStatusChanged() { root.rebuildSourcesModel() }
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
                                    model: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.cameras : [] )
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
                                    enabled: ( appSupervisor && appSupervisor.deviceCatalogue && appSupervisor.deviceCatalogue.cameras.length > 0 )
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
                                    visible: ( root.cameraPreviewHandle !== null )
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: {
                                        if ( !appSupervisor || !appSupervisor.deviceCatalogue || appSupervisor.deviceCatalogue.cameras.length <= 0 ) {
                                            return "No camera devices detected."
                                        }
                                        if ( !root.cameraPreviewActive ) {
                                            return "Preview is stopped."
                                        }
                                        if ( root.cameraPreviewHandle && root.cameraPreviewHandle.errorText && root.cameraPreviewHandle.errorText.length > 0 ) {
                                            return root.cameraPreviewHandle.errorText
                                        }
                                        return "Starting camera..."
                                    }
                                    visible: ( root.cameraPreviewHandle === null )
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
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: 6
                                        color: ( theme ? theme.panelInsetColour : tintSoft )

                                        ListView {
                                            id: sourcesList
                                            objectName: "sourcesList"
                                            anchors.fill: parent
                                            anchors.margins: ( theme ? theme.spaceCompact : 8 )
                                            clip: true
                                            model: sourceModel
                                            spacing: 6

                                            ScrollBar.vertical: VcScrollBar {
                                                id: sourcesScrollBar
                                                objectName: "sourcesScrollBar"
                                                theme: root.theme
                                                policy: ScrollBar.AsNeeded
                                                orientation: Qt.Vertical
                                                anchors.right: parent.right
                                                anchors.top: parent.top
                                                anchors.bottom: parent.bottom
                                            }

                                            delegate: Rectangle {
                                                width: ListView.view.width
                                                    - ( sourcesScrollBar.visible
                                                        ? ( sourcesScrollBar.width + ( theme ? theme.spaceNudge : 4 ) )
                                                        : 0 )
                                                height: {
                                                    if ( rowType === "header" ) {
                                                        return ( theme ? Math.max( 22, theme.fontBasePx + 6 ) : 24 )
                                                    }
                                                    if ( rowType === "note" ) {
                                                        return ( theme ? Math.max( 22, theme.fontBasePx + 6 ) : 24 )
                                                    }
                                                    return ( theme ? theme.compactControlHeight : 34 )
                                                }
                                                radius: 6
                                                color: ( rowType === "device" ? tintSofter : "transparent" )

                                                objectName: {
                                                    if ( rowType === "header" ) {
                                                        return "sourceHeader_" + sectionKey
                                                    }
                                                    if ( rowType === "note" ) {
                                                        return "sourceNote_" + sectionKey
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
                                                            elide: Text.ElideRight
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
                                                    visible: ( rowType !== "header" )

                                                    Label {
                                                        text: label
                                                        Layout.fillWidth: true
                                                        elide: Text.ElideRight
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
                                                        root.openSourceInspectorDevice( kind, deviceId, label )
                                                    }
                                                }
                                            }
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
