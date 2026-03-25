import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Control {
    id: root
    objectName: "preferencesOverlay"

    property bool open: false
    property int currentCategoryIndex: 0
    property var appPalette
    property var theme
    property var fontFamiliesCache: []

    signal closeRequested()

    visible: open
    z: 20000
    palette: appPalette

    readonly property var pal: root.palette
    readonly property color scrimColour: ( theme ? theme.scrimColour : Qt.rgba( pal.shadow.r, pal.shadow.g, pal.shadow.b, 0.35 ) )

    onOpenChanged: {
        if ( open && appSupervisor && appSupervisor.deviceCatalogue ) {
            appSupervisor.deviceCatalogue.refresh()
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
                        accentRole: "primary"

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
                                model: [
                                    {
                                        title: "Screens",
                                        items: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.screens : [] ),
                                        detailKey: "availableGeometry",
                                        emptyText: "No screens detected."
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
                                        title: "Audio outputs",
                                        items: ( appSupervisor && appSupervisor.deviceCatalogue ? appSupervisor.deviceCatalogue.audioOutputs : [] ),
                                        defaultSuffix: true,
                                        emptyText: "No audio outputs detected."
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
                                    accentRole: ( index === 0 ? "secondary" : "tertiary" )
                                    property var sectionData: modelData

                                    ColumnLayout {
                                        width: parent.width
                                        spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                                        Label {
                                            text: deviceSectionCard.sectionData.title
                                            font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                            color: ( root.theme ? root.theme.textColour : root.pal.text )
                                        }

                                        Label {
                                            visible: ( deviceSectionCard.sectionData.statusText !== undefined && deviceSectionCard.sectionData.statusText.length > 0 )
                                            text: deviceSectionCard.sectionData.statusText !== undefined ? deviceSectionCard.sectionData.statusText : ""
                                            wrapMode: Text.Wrap
                                            Layout.fillWidth: true
                                            color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                        }

                                        Repeater {
                                            model: deviceSectionCard.sectionData.items

                                            delegate: Label {
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

                                        Label {
                                            visible: deviceSectionCard.sectionData.items.length === 0
                                            text: deviceSectionCard.sectionData.emptyText
                                            wrapMode: Text.Wrap
                                            color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
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
        width: Math.min( parent.width * 0.92, 820 )
        height: Math.min( parent.height * 0.9, 620 )
        theme: root.theme
        accentRole: "secondary"

        ColumnLayout {
            anchors.fill: parent
            spacing: ( root.theme ? root.theme.spaceTight : 12 )

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: "Preferences"
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
                        objectName: "preferencesCategoryList"
                        anchors.fill: parent
                        clip: true
                        spacing: ( root.theme ? root.theme.spaceNudge : 6 )
                        model: categoryModel
                        currentIndex: root.currentCategoryIndex

                        delegate: Rectangle {
                            objectName: model.objectName
                            width: ListView.view.width
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
        }
    }
}
