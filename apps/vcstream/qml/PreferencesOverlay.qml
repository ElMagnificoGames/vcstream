import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtMultimedia 6.0

Control {
    id: root
    objectName: "preferencesOverlay"

    property bool open: false
    property int currentCategoryIndex: 0
    property var appPalette
    property var theme
    property var fontFamiliesCache: []

    // Camera preview (device registry is owned by AppSupervisor).
    property bool cameraPreviewOpen: false
    property string cameraPreviewDeviceId: ""
    property bool cameraPreviewActive: false
    property var cameraPreviewHandle: null

    signal closeRequested()

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

    function openCameraPreview() {
        ensureCameraPreviewDeviceSelected()
        if ( cameraPreviewDeviceId.length <= 0 ) {
            return
        }

        cameraPreviewOpen = true
        cameraPreviewActive = false
    }

    function closeCameraPreview() {
        stopCameraPreview()
        cameraPreviewOpen = false
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

        cameraPreviewHandle = appSupervisor.mediaCapture.acquireCameraPreviewHandle( cameraPreviewDeviceId, panel )
        if ( cameraPreviewHandle ) {
            cameraPreviewHandle.setViewSink( preferencesCameraPreviewVideo.videoSink )
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

    visible: open
    z: 20000
    palette: appPalette

    readonly property var pal: root.palette
    readonly property color scrimColour: ( theme ? theme.scrimColour : Qt.rgba( pal.shadow.r, pal.shadow.g, pal.shadow.b, 0.35 ) )

    onOpenChanged: {
        if ( open && appSupervisor && appSupervisor.deviceCatalogue ) {
            appSupervisor.deviceCatalogue.refresh()
        }

        if ( !open ) {
            closeCameraPreview()
        }

        if ( open && appSupervisor && appSupervisor.fontFamilies ) {
            fontFamiliesCache = appSupervisor.fontFamilies()
        }
    }

    onCurrentCategoryIndexChanged: {
        if ( open && currentCategoryIndex === 1 && appSupervisor && appSupervisor.deviceCatalogue ) {
            appSupervisor.deviceCatalogue.refresh()
        }

        if ( open && currentCategoryIndex === 0 && appSupervisor && appSupervisor.fontFamilies ) {
            fontFamiliesCache = appSupervisor.fontFamilies()
        }
    }

    Component {
        id: generalPaneComponent

        Item {
            objectName: "preferencesGeneralPane"

            ScrollView {
                id: generalScrollView
                objectName: "preferencesGeneralScrollView"
                anchors.fill: parent
                clip: true

                // Always reserve space so the scroll bar cannot cover content.
                rightPadding: ( generalScrollBar.width + 12 )

                // Disable Qt style/plugin scroll bars and indicators.
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ScrollBar.vertical: VcScrollBar {
                    id: generalScrollBar
                    objectName: "preferencesGeneralScrollBar"
                    theme: root.theme
                    policy: ScrollBar.AsNeeded
                    height: generalScrollView.height
                    x: generalScrollView.width - width
                    y: 0
                }

                contentItem: Flickable {
                    id: generalFlickable
                    width: generalScrollView.availableWidth
                    height: generalScrollView.availableHeight
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.VerticalFlick

                    ScrollIndicator.horizontal: ScrollIndicator { visible: false }
                    ScrollIndicator.vertical: ScrollIndicator { visible: false }

                    contentWidth: width
                    contentHeight: generalColumn.implicitHeight

                    Column {
                        id: generalColumn
                        width: generalFlickable.width
                        spacing: ( root.theme ? root.theme.spaceTight : 12 )

                    VcPanel {
                        width: parent.width
                        theme: root.theme
                        accentRole: "tertiary"

                        ColumnLayout {
                            width: parent.width
                            spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                            Label {
                                text: "Display name"
                                font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                            }

                            VcTextField {
                                objectName: "preferencesDisplayNameField"
                                theme: root.theme
                                Layout.fillWidth: true
                                placeholderText: "Your name"
                                text: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.displayName : "" )

                                onEditingFinished: {
                                    if ( appSupervisor && appSupervisor.preferences ) {
                                        appSupervisor.preferences.displayName = text
                                    }
                                }
                            }
                        }
                    }

                    VcPanel {
                        width: parent.width
                        theme: root.theme
                        accentRole: "tertiary"

                        ColumnLayout {
                            width: parent.width
                            spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                            Label {
                                text: "Typeface"
                                font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                Label {
                                    text: "Preset"
                                    color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                }

                                VcComboBox {
                                    id: fontPresetCombo
                                    objectName: "preferencesFontPresetCombo"
                                    theme: root.theme
                                    Layout.fillWidth: true
                                    model: ["Victorian", "System default", "Custom..."]
                                    currentIndex: indexForPresetValue( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.fontPreset : "victorian" )

                                    function presetValueForIndex( idx ) {
                                        if ( idx === 2 ) return "custom"
                                        if ( idx === 1 ) return "system"
                                        return "victorian"
                                    }

                                    function indexForPresetValue( v ) {
                                        if ( v === "system" ) return 1
                                        if ( v === "custom" ) return 2
                                        return 0
                                    }

                                    onActivated: {
                                        if ( appSupervisor && appSupervisor.preferences ) {
                                            appSupervisor.preferences.fontPreset = presetValueForIndex( currentIndex )
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: ( appSupervisor && appSupervisor.preferences && appSupervisor.preferences.fontPreset === "custom" )
                                spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                Label {
                                    text: "Font"
                                    color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                }

                                VcFontPicker {
                                    id: fontFamilyPicker
                                    objectName: "preferencesFontFamilyPicker"
                                    theme: root.theme
                                    appPalette: root.appPalette
                                    fontPreviewSafetyCache: ( appSupervisor ? appSupervisor.fontPreviewSafetyCache : null )
                                    systemFontFamily: ( appSupervisor ? appSupervisor.systemFontFamily : "" )
                                    Layout.fillWidth: true
                                    families: root.fontFamiliesCache
                                    value: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.fontFamily : "" )
                                    sampleText: "Aa Bb Cc Dd Ee Ff 0123456789"

                                    onValueSelected: function( v ) {
                                        if ( appSupervisor && appSupervisor.preferences ) {
                                            appSupervisor.preferences.fontFamily = v

                                            if ( v.length === 0 ) {
                                                appSupervisor.preferences.fontPreset = "system"
                                            } else {
                                                appSupervisor.preferences.fontPreset = "custom"
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                objectName: "preferencesFontSampleBox"
                                Layout.fillWidth: true
                                Layout.preferredHeight: ( root.theme ? root.theme.controlHeight : 42 )
                                radius: ( root.theme ? root.theme.controlRadius : 10 )
                                color: ( root.theme ? root.theme.panelInsetColour : root.pal.base )
                                border.color: ( root.theme ? root.theme.frameColour : root.pal.mid )
                                border.width: 1
                                clip: true

                                Label {
                                    id: fontSampleLabel
                                    anchors.fill: parent
                                    anchors.margins: ( root.theme ? root.theme.spaceCompact : 8 )
                                    wrapMode: Text.NoWrap
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                    verticalAlignment: Text.AlignVCenter
                                    readonly property string selectedFamily: {
                                        if ( !appSupervisor || !appSupervisor.preferences ) {
                                            return ""
                                        }

                                        if ( appSupervisor.preferences.fontPreset === "custom" ) {
                                            return appSupervisor.preferences.fontFamily
                                        }

                                        if ( appSupervisor.preferences.fontPreset === "victorian" ) {
                                            return ( appSupervisor.victorianBodyFontFamily ? appSupervisor.victorianBodyFontFamily : "" )
                                        }

                                        return ""
                                    }

                                    property int vcHealthSerial: 0

                                    readonly property int vcSelectedStatus: {
                                        var _ = vcHealthSerial
                                        if ( appSupervisor && appSupervisor.fontPreviewSafetyCache && selectedFamily.length > 0 ) {
                                            return appSupervisor.fontPreviewSafetyCache.statusForFamily( selectedFamily, ( root.theme ? root.theme.fontBasePx : 14 ) )
                                        }
                                        return 2
                                    }

                                    function vcEnsureChecked() {
                                        if ( appSupervisor && appSupervisor.fontPreviewSafetyCache && selectedFamily.length > 0 ) {
                                            appSupervisor.fontPreviewSafetyCache.requestCheck( selectedFamily, ( root.theme ? root.theme.fontBasePx : 14 ) )
                                        }
                                    }

                                    Component.onCompleted: vcEnsureChecked()
                                    onSelectedFamilyChanged: vcEnsureChecked()

                                    Connections {
                                        target: ( appSupervisor ? appSupervisor.fontPreviewSafetyCache : null )

                                        function onStatusChanged( family, pixelSize, status ) {
                                            fontSampleLabel.vcHealthSerial = fontSampleLabel.vcHealthSerial + 1
                                        }
                                    }

                                    text: {
                                        if ( vcSelectedStatus === 1 ) return "Checking preview..."
                                        if ( vcSelectedStatus === 3 ) return "Preview unavailable: this font may not render correctly on this device"
                                        if ( vcSelectedStatus === 4 ) return "Preview unavailable right now"
                                        if ( vcSelectedStatus === 5 ) return "Preview paused to keep the app responsive"
                                        return "Aa Bb Cc Dd Ee Ff 0123456789"
                                    }
                                    color: ( vcSelectedStatus === 3 || vcSelectedStatus === 4 || vcSelectedStatus === 5 )
                                        ? ( root.theme ? root.theme.dangerColour : root.pal.text )
                                        : ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                    font.family: ( vcSelectedStatus === 2 && selectedFamily.length > 0 )
                                        ? selectedFamily
                                        : ( appSupervisor ? appSupervisor.systemFontFamily : Qt.application.font.family )
                                    font.bold: ( vcSelectedStatus === 3 || vcSelectedStatus === 4 || vcSelectedStatus === 5 )
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: ( root.theme ? root.theme.spaceNudge : 4 )

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                    Label {
                                        text: "Font size"
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                    }

                                    Label {
                                        text: ( appSupervisor && appSupervisor.preferences ? ( appSupervisor.preferences.fontScalePercent + "%" ) : "" )
                                        color: ( root.theme ? root.theme.textColour : root.pal.text )
                                    }
                                }

                                VcSlider {
                                    objectName: "preferencesFontScaleSlider"
                                    theme: root.theme
                                    Layout.fillWidth: true
                                    from: 75
                                    to: 150
                                    stepSize: 5
                                    snapMode: Slider.SnapAlways
                                    value: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.fontScalePercent : 100 )

                                    onMoved: {
                                        if ( appSupervisor && appSupervisor.preferences ) {
                                            appSupervisor.preferences.fontScalePercent = Math.round( value )
                                        }
                                    }
                                }
                            }

                        }
                    }

                    VcPanel {
                        width: parent.width
                        theme: root.theme
                        accentRole: "tertiary"

                        ColumnLayout {
                            width: parent.width
                            spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                            Label {
                                text: "Layout"
                                font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                Label {
                                    text: "Density"
                                    color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                }

                                VcComboBox {
                                    id: densityCombo
                                    objectName: "preferencesDensityCombo"
                                    theme: root.theme
                                    Layout.fillWidth: true
                                    model: ["Compact", "Comfortable", "Spacious"]
                                    currentIndex: indexForDensityValue( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.density : "comfortable" )

                                    function densityValueForIndex( idx ) {
                                        if ( idx === 0 ) return "compact"
                                        if ( idx === 2 ) return "spacious"
                                        return "comfortable"
                                    }

                                    function indexForDensityValue( v ) {
                                        if ( v === "compact" ) return 0
                                        if ( v === "spacious" ) return 2
                                        return 1
                                    }

                                    onActivated: {
                                        if ( appSupervisor && appSupervisor.preferences ) {
                                            appSupervisor.preferences.density = densityValueForIndex( currentIndex )
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: ( root.theme ? root.theme.spaceNudge : 4 )

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                    Label {
                                        text: "Zoom"
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                    }

                                    Label {
                                        text: ( appSupervisor && appSupervisor.preferences ? ( appSupervisor.preferences.zoomPercent + "%" ) : "" )
                                        color: ( root.theme ? root.theme.textColour : root.pal.text )
                                    }
                                }

                                VcSlider {
                                    objectName: "preferencesZoomSlider"
                                    theme: root.theme
                                    Layout.fillWidth: true
                                    from: 50
                                    to: 200
                                    stepSize: 5
                                    snapMode: Slider.SnapAlways
                                    value: ( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.zoomPercent : 100 )

                                    onMoved: {
                                        if ( appSupervisor && appSupervisor.preferences ) {
                                            appSupervisor.preferences.zoomPercent = Math.round( value )
                                        }
                                    }
                                }
                            }
                        }
                    }

                    VcPanel {
                        width: parent.width
                        theme: root.theme
                        accentRole: "none"

                        ColumnLayout {
                            width: parent.width
                            spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                            Label {
                                text: "Theme"
                                font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                            }

                            RowLayout {
                                width: parent.width
                                spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                Label {
                                    text: "Mode"
                                    color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                }

                                 VcComboBox {
                                      objectName: "preferencesThemeModeCombo"
                                      theme: root.theme
                                      Layout.fillWidth: true
                                      model: ["Victorian", "System", "Light", "Dark"]
                                      currentIndex: indexForModeValue( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.themeMode : "victorian" )

                                      function modeValueForIndex( idx ) {
                                          if ( idx === 3 ) return "dark"
                                          if ( idx === 2 ) return "light"
                                          if ( idx === 1 ) return "system"
                                          return "victorian"
                                      }

                                      function indexForModeValue( v ) {
                                          if ( v === "system" ) return 1
                                          if ( v === "light" ) return 2
                                          if ( v === "dark" ) return 3
                                          return 0
                                      }

                                    onActivated: {
                                        if ( appSupervisor && appSupervisor.preferences ) {
                                            appSupervisor.preferences.themeMode = modeValueForIndex( currentIndex )
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                width: parent.width
                                spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                Label {
                                    text: "Accent"
                                    color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                }

                                 VcComboBox {
                                      objectName: "preferencesAccentCombo"
                                      theme: root.theme
                                      Layout.fillWidth: true
                                      model: ["Victorian", "System", "Red", "Orange", "Yellow", "Green", "Cyan", "Blue", "Pink", "Purple", "Custom..."]
                                      currentIndex: indexForAccentValue( appSupervisor && appSupervisor.preferences ? appSupervisor.preferences.accent : "victorian" )

                                      function accentValueForIndex( idx ) {
                                          if ( idx === 10 ) return "custom"
                                          if ( idx === 9 ) return "purple"
                                          if ( idx === 8 ) return "pink"
                                          if ( idx === 7 ) return "blue"
                                          if ( idx === 6 ) return "cyan"
                                          if ( idx === 5 ) return "green"
                                          if ( idx === 4 ) return "yellow"
                                          if ( idx === 3 ) return "orange"
                                          if ( idx === 2 ) return "red"
                                          if ( idx === 1 ) return "system"
                                          return "victorian"
                                      }

                                      function indexForAccentValue( v ) {
                                          if ( v === "system" ) return 1
                                          if ( v === "red" ) return 2
                                          if ( v === "orange" ) return 3
                                          if ( v === "yellow" ) return 4
                                          if ( v === "green" ) return 5
                                          if ( v === "cyan" ) return 6
                                          if ( v === "blue" ) return 7
                                          if ( v === "pink" ) return 8
                                          if ( v === "purple" ) return 9
                                          if ( v === "custom" ) return 10
                                          return 0
                                      }

                                    onActivated: {
                                        if ( appSupervisor && appSupervisor.preferences ) {
                                            appSupervisor.preferences.accent = accentValueForIndex( currentIndex )
                                        }
                                    }
                                }

                                Rectangle {
                                    objectName: "preferencesAccentSwatch"
                                    Layout.preferredWidth: 18
                                    Layout.preferredHeight: 18
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 4
                                    color: ( root.theme ? root.theme.primaryAccentColour : root.pal.highlight )
                                    border.color: ( root.theme ? root.theme.frameColour : root.pal.mid )
                                    border.width: 1
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: ( appSupervisor && appSupervisor.preferences && appSupervisor.preferences.accent === "custom" )
                                spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 18
                                    radius: 4
                                    color: ( typeof oklchUtil !== "undefined" && appSupervisor && appSupervisor.preferences )
                                        ? oklchUtil.oklchToSrgbFitted(
                                            appSupervisor.preferences.customAccentLightness,
                                            appSupervisor.preferences.customAccentChroma,
                                            appSupervisor.preferences.customAccentHueDegrees )
                                        : root.pal.highlight
                                    border.color: ( root.theme ? root.theme.frameColour : root.pal.mid )
                                    border.width: 1
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 22

                                    Image {
                                        objectName: "preferencesAccentHueBar"
                                        anchors.fill: parent
                                        fillMode: Image.Stretch
                                        source: "image://vcTheme/hue"
                                        sourceSize.width: Math.round( width )
                                        sourceSize.height: Math.round( height )
                                    }

                                    MouseArea {
                                        objectName: "preferencesHueBarMouseArea"
                                        anchors.fill: parent
                                        hoverEnabled: true

                                        onWheel: function( wheel ) {
                                            wheel.accepted = false
                                        }

                                        onPressed: function( mouse ) {
                                            if ( appSupervisor && appSupervisor.preferences ) {
                                                var t = mouse.x / Math.max( 1, width )
                                                appSupervisor.preferences.customAccentHueDegrees = Math.max( 0, Math.min( 359.999, t * 360.0 ) )
                                            }
                                        }

                                        onPositionChanged: function( mouse ) {
                                            if ( pressed && appSupervisor && appSupervisor.preferences ) {
                                                var t = mouse.x / Math.max( 1, width )
                                                appSupervisor.preferences.customAccentHueDegrees = Math.max( 0, Math.min( 359.999, t * 360.0 ) )
                                            }
                                        }
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 180

                                    readonly property real maxC: 0.37
                                    readonly property real minL: 0.08
                                    readonly property real maxL: 0.95

                                    Image {
                                        objectName: "preferencesAccentPlane"
                                        anchors.fill: parent
                                        fillMode: Image.Stretch
                                        source: ( appSupervisor && appSupervisor.preferences
                                            ? "image://vcTheme/plane/" + Math.round( appSupervisor.preferences.customAccentHueDegrees )
                                            : "image://vcTheme/plane/0" )
                                        cache: false
                                        sourceSize.width: Math.round( width )
                                        sourceSize.height: Math.round( height )
                                    }

                                    MouseArea {
                                        objectName: "preferencesAccentPlaneMouseArea"
                                        anchors.fill: parent
                                        hoverEnabled: true

                                        onWheel: function( wheel ) {
                                            wheel.accepted = false
                                        }

                                        onPressed: function( mouse ) {
                                            if ( appSupervisor && appSupervisor.preferences ) {
                                                var tx = mouse.x / Math.max( 1, width )
                                                var ty = mouse.y / Math.max( 1, height )
                                                appSupervisor.preferences.customAccentChroma = Math.max( 0, Math.min( parent.maxC, tx * parent.maxC ) )
                                                appSupervisor.preferences.customAccentLightness = Math.max( parent.minL, Math.min( parent.maxL, parent.minL + ( 1.0 - ty ) * ( parent.maxL - parent.minL ) ) )
                                            }
                                        }

                                        onPositionChanged: function( mouse ) {
                                            if ( pressed && appSupervisor && appSupervisor.preferences ) {
                                                var tx = mouse.x / Math.max( 1, width )
                                                var ty = mouse.y / Math.max( 1, height )
                                                appSupervisor.preferences.customAccentChroma = Math.max( 0, Math.min( parent.maxC, tx * parent.maxC ) )
                                                appSupervisor.preferences.customAccentLightness = Math.max( parent.minL, Math.min( parent.maxL, parent.minL + ( 1.0 - ty ) * ( parent.maxL - parent.minL ) ) )
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                        Item { width: parent.width; height: 6 }
                    }
                }
            }
        }
    }

    Component {
        id: devicesPaneComponent

        Item {
            objectName: "preferencesDevicesPane"

            ColumnLayout {
                anchors.fill: parent
                spacing: ( root.theme ? root.theme.spaceTight : 12 )

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Local devices"
                        font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        color: ( root.theme ? root.theme.textColour : root.pal.text )
                    }

                    VcButton {
                        objectName: "preferencesDeviceRefreshButton"
                        theme: root.theme
                        tone: "secondary"
                        compact: true
                        text: "Refresh"

                        onClicked: {
                            if ( appSupervisor && appSupervisor.deviceCatalogue ) {
                                appSupervisor.deviceCatalogue.refresh()
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollView {
                        id: devicesScrollView
                        objectName: "preferencesDevicesScrollView"
                        anchors.fill: parent
                        clip: true

                        // Always reserve space so the scroll bar cannot cover content.
                        rightPadding: ( devicesScrollBar.width + 12 )

                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        ScrollBar.vertical: VcScrollBar {
                            id: devicesScrollBar
                            objectName: "preferencesDevicesScrollBar"
                            theme: root.theme
                            policy: ScrollBar.AsNeeded
                            height: devicesScrollView.height
                            x: devicesScrollView.width - width
                            y: 0
                        }

                        contentItem: Flickable {
                            id: devicesFlickable
                            width: devicesScrollView.availableWidth
                            height: devicesScrollView.availableHeight
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.VerticalFlick

                            ScrollIndicator.horizontal: ScrollIndicator { visible: false }
                            ScrollIndicator.vertical: ScrollIndicator { visible: false }

                            contentWidth: width
                            contentHeight: devicesColumn.implicitHeight

                            Column {
                                id: devicesColumn
                                width: devicesFlickable.width
                                spacing: ( root.theme ? root.theme.spaceTight : 12 )

                            Repeater {
                                // Sorted alphabetically by section title.
                                model: [
                                    {
                                        title: "Audio outputs",
                                        items: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.audioOutputs : [] ),
                                        defaultSuffix: true,
                                        emptyText: "No audio outputs detected."
                                    },
                                    {
                                        title: "Cameras",
                                        items: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.cameras : [] ),
                                        defaultSuffix: true,
                                        emptyText: "No cameras detected."
                                    },
                                    {
                                        title: "Microphones",
                                        items: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.microphones : [] ),
                                        defaultSuffix: true,
                                        emptyText: "No microphones detected."
                                    },
                                    {
                                        title: "Screens",
                                        items: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.screens : [] ),
                                        detailKey: "availableGeometry",
                                        emptyText: "No screens detected."
                                    },
                                    {
                                        title: "Windows",
                                        items: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.windows : [] ),
                                        statusText: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.windowCaptureStatus : "" ),
                                        emptyText: "No capturable windows reported."
                                    }
                                ]

                                delegate: VcPanel {
                                    id: deviceSectionCard
                                    width: parent.width
                                    theme: root.theme
                                    accentRole: "none"
                                    property var sectionData: modelData

                                    ColumnLayout {
                                        width: parent.width
                                        spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                        RowLayout {
                                            Layout.fillWidth: true

                                            Label {
                                                text: deviceSectionCard.sectionData.title
                                                font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                                                Layout.fillWidth: true
                                                elide: Text.ElideRight
                                            }
                                        }

                                        Label {
                                            visible: ( deviceSectionCard.sectionData.statusText !== undefined && deviceSectionCard.sectionData.statusText.length > 0 )
                                            text: deviceSectionCard.sectionData.statusText !== undefined ? deviceSectionCard.sectionData.statusText : ""
                                            wrapMode: Text.Wrap
                                            Layout.fillWidth: true
                                            color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                        }

                                        Rectangle {
                                            id: deviceSectionListFrame
                                            Layout.fillWidth: true
                                            radius: 6
                                            color: ( root.theme ? root.theme.panelInsetColour : Qt.rgba( root.pal.text.r, root.pal.text.g, root.pal.text.b, 0.04 ) )
                                            border.color: ( root.theme ? root.theme.frameInnerColour : root.pal.midlight )
                                            border.width: 1
                                            clip: true

                                            readonly property int listPadding: ( root.theme ? root.theme.spaceCompact : 8 )
                                            implicitHeight: ( sectionList.implicitHeight + ( deviceSectionListFrame.listPadding * 2 ) )

                                            Column {
                                                id: sectionList
                                                anchors.fill: parent
                                                anchors.margins: deviceSectionListFrame.listPadding
                                                spacing: ( root.theme ? root.theme.spaceNudge : 6 )

                                                Repeater {
                                                    model: deviceSectionCard.sectionData.items

                                                    delegate: Rectangle {
                                                        id: deviceRow
                                                        width: sectionList.width
                                                        radius: 6

                                                        readonly property bool isCameraRow: ( deviceSectionCard.sectionData.title === "Cameras" )
                                                        property bool testSkipActivate: true

                                                        objectName: isCameraRow
                                                            ? ( "preferencesCameraRow_" + modelData.id )
                                                            : ""

                                                        color: mouseArea.containsMouse && mouseArea.enabled
                                                            ? ( root.theme ? root.theme.panelColour : Qt.rgba( root.pal.text.r, root.pal.text.g, root.pal.text.b, 0.06 ) )
                                                            : "transparent"

                                                        border.color: mouseArea.containsMouse && mouseArea.enabled
                                                            ? ( root.theme ? root.theme.frameColour : root.pal.mid )
                                                            : "transparent"
                                                        border.width: mouseArea.containsMouse && mouseArea.enabled ? 1 : 0

                                                        implicitHeight: Math.max( nameLabel.implicitHeight + 12,
                                                            ( root.theme ? root.theme.compactControlHeight - 4 : 30 ) )

                                                        RowLayout {
                                                            anchors.fill: parent
                                                            anchors.leftMargin: 8
                                                            anchors.rightMargin: 8
                                                            anchors.topMargin: 6
                                                            anchors.bottomMargin: 6
                                                            spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                                            Label {
                                                                id: nameLabel
                                                                Layout.fillWidth: true
                                                                wrapMode: Text.Wrap
                                                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                                                                text: {
                                                                    var out = modelData.name
                                                                    if ( deviceSectionCard.sectionData.defaultSuffix && modelData.isDefault ) {
                                                                        out += " (default)"
                                                                    }
                                                                    if ( deviceSectionCard.sectionData.detailKey && modelData[deviceSectionCard.sectionData.detailKey] ) {
                                                                        out += "  " + modelData[deviceSectionCard.sectionData.detailKey]
                                                                    }
                                                                    return out
                                                                }
                                                            }
                                                        }

                                                        MouseArea {
                                                            id: mouseArea
                                                            anchors.fill: parent
                                                            enabled: deviceRow.isCameraRow
                                                            hoverEnabled: true
                                                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

                                                            onClicked: {
                                                                root.cameraPreviewDeviceId = modelData.id
                                                                root.openCameraPreview()
                                                            }
                                                        }
                                                    }
                                                }

                                                Label {
                                                    visible: deviceSectionCard.sectionData.items.length === 0
                                                    width: sectionList.width
                                                    wrapMode: Text.Wrap
                                                    text: deviceSectionCard.sectionData.emptyText
                                                    color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                                }
                                            }
                                        }

                                        // Empty text is rendered inside the list frame.
                                    }
                                }
                            }

                                Item { width: parent.width; height: 6 }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: scrimColour

        MouseArea {
            anchors.fill: parent

            onClicked: function( mouse ) {
                if ( mouse.x >= panel.x && mouse.x <= ( panel.x + panel.width )
                    && mouse.y >= panel.y && mouse.y <= ( panel.y + panel.height ) ) {
                    return
                }

                root.closeRequested()
            }
        }
    }

    VcPanel {
        id: panel
        objectName: "preferencesPanel"
        anchors.centerIn: parent
        width: root.cameraPreviewOpen
            ? Math.min( parent.width * 0.98, ( root.theme && root.theme.macroPx ? root.theme.macroPx( 1280 ) : 1280 ) )
            : Math.min( parent.width * 0.92, 820 )
        height: root.cameraPreviewOpen
            ? Math.min( parent.height * 0.96, ( root.theme && root.theme.macroPx ? root.theme.macroPx( 900 ) : 900 ) )
            : Math.min( parent.height * 0.9, 620 )
        theme: root.theme
        accentRole: "secondary"

        ColumnLayout {
            anchors.fill: parent
            spacing: ( root.theme ? root.theme.spaceTight : 12 )

            RowLayout {
                Layout.fillWidth: true

                VcButton {
                    visible: root.cameraPreviewOpen
                    objectName: "preferencesCameraPreviewBackButton"
                    theme: root.theme
                    tone: "secondary"
                    compact: true
                    text: "Back"

                    onClicked: root.closeCameraPreview()
                }

                Label {
                    text: ( root.cameraPreviewOpen ? "Camera preview" : "Preferences" )
                    font.pixelSize: ( root.theme ? root.theme.fontHeadingPx : 18 )
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    color: ( root.theme ? root.theme.textColour : root.pal.text )
                }

                VcCloseButton {
                    objectName: "preferencesCloseButton"
                    theme: root.theme
                    onClicked: root.closeRequested()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: ( root.theme ? root.theme.spaceTight : 12 )
                visible: !root.cameraPreviewOpen

                VcPanel {
                    Layout.preferredWidth: ( root.theme && root.theme.macroPx ? root.theme.macroPx( 220 ) : 220 )
                    Layout.minimumWidth: ( root.theme && root.theme.macroPx ? root.theme.macroPx( 180 ) : 180 )
                    Layout.maximumWidth: ( root.theme && root.theme.macroPx ? root.theme.macroPx( 260 ) : 260 )
                    Layout.fillHeight: true
                    theme: root.theme
                    inset: true
                    padding: ( root.theme ? root.theme.spaceCompact : 8 )

                    ListModel {
                        id: categoryModel
                        ListElement { title: "Appearance"; objectName: "preferencesCategoryGeneral" }
                        ListElement { title: "Devices"; objectName: "preferencesCategoryDevices" }
                    }

                    ListView {
                        id: categoryList
                        objectName: "preferencesCategoryList"
                        anchors.fill: parent
                        clip: true
                        spacing: ( root.theme ? root.theme.spaceNudge : 6 )
                        model: categoryModel
                        currentIndex: root.currentCategoryIndex

                        ScrollBar.vertical: VcScrollBar {
                            id: categoryScrollBar
                            objectName: "preferencesCategoryScrollBar"
                            theme: root.theme
                            policy: ScrollBar.AsNeeded
                            orientation: Qt.Vertical
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                        }

                        delegate: Rectangle {
                            objectName: model.objectName
                            width: categoryList.width - ( categoryScrollBar.visible ? ( categoryScrollBar.width + 6 ) : 0 )
                            height: ( root.theme ? root.theme.controlHeight : 42 )
                            radius: ( root.theme ? root.theme.controlRadius : 10 )
                            color: ListView.isCurrentItem
                                ? ( root.theme ? root.theme.tertiaryAccentColour : root.pal.highlight )
                                : "transparent"
                            border.color: ListView.isCurrentItem
                                ? ( root.theme ? root.theme.frameColour : root.pal.mid )
                                : "transparent"
                            border.width: ListView.isCurrentItem ? 1 : 0

                            Label {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                verticalAlignment: Text.AlignVCenter
                                text: model.title
                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.currentCategoryIndex = index
                            }
                        }
                    }
                }

                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sourceComponent: ( root.currentCategoryIndex === 1 ? devicesPaneComponent : generalPaneComponent )
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.cameraPreviewOpen

                Rectangle {
                    anchors.fill: parent
                    radius: ( root.theme ? root.theme.panelRadius : 8 )
                    color: ( root.theme ? root.theme.panelInsetColour : Qt.rgba( root.pal.text.r, root.pal.text.g, root.pal.text.b, 0.05 ) )
                    border.color: ( root.theme ? root.theme.frameInnerColour : root.pal.midlight )
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ( root.theme ? root.theme.spaceCompact : 10 )
                        spacing: ( root.theme ? root.theme.spaceCompact : 10 )

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                            Label {
                                text: "Camera"
                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                            }

                            VcComboBox {
                                id: preferencesCameraPreviewCombo
                                objectName: "preferencesCameraPreviewCombo"
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
                                    }
                                }
                            }

                            VcButton {
                                id: preferencesCameraPreviewToggleButton
                                objectName: "preferencesCameraPreviewToggleButton"
                                theme: root.theme
                                tone: ( root.cameraPreviewActive ? "secondary" : "primary" )
                                text: ( root.cameraPreviewActive ? "Stop preview" : "Start preview" )
                                enabled: ( appSupervisor && appSupervisor.deviceCatalogue && appSupervisor.deviceCatalogue.cameras.length > 0 )
                                hoverEnabled: true
                                property bool testSkipActivate: true

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
                            id: preferencesCameraPreviewFrame
                            objectName: "preferencesCameraPreviewFrame"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: ( root.theme ? root.theme.panelRadius : 8 )
                            color: Qt.rgba( root.pal.shadow.r, root.pal.shadow.g, root.pal.shadow.b, 0.08 )
                            border.color: ( root.theme ? root.theme.frameInnerColour : root.pal.midlight )
                            border.width: 1

                            VideoOutput {
                                id: preferencesCameraPreviewVideo
                                objectName: "preferencesCameraPreviewVideo"
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
                                color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                            }
                        }

                        VcPanel {
                            id: preferencesCameraPreviewDetailsPanel
                            objectName: "preferencesCameraPreviewDetailsPanel"
                            Layout.fillWidth: true
                            theme: root.theme
                            accentRole: "tertiary"

                            ColumnLayout {
                                id: preferencesCameraDetailsLayout
                                width: parent.width
                                spacing: 6

                                Label {
                                    text: "Details"
                                    font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                    color: ( root.theme ? root.theme.textColour : root.pal.text )
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
                                    text: preferencesCameraDetailsLayout.vcCameraOriginText
                                    wrapMode: Text.Wrap
                                    color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                }

                                Label {
                                    text: "Owner: This device"
                                    wrapMode: Text.Wrap
                                    color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
