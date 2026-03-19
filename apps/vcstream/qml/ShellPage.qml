import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

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

    signal disconnectRequested()
    signal preferencesRequested()

    property int selectedSourceIndex: -1
    property string selectedSourceName: ""
    property bool selectedSourceExportEnabled: false
    property bool sourceInspectorOpen: false

    function openSourceInspector( index ) {
        if ( index < 0 || index >= sourceModel.count ) {
            return
        }

        selectedSourceIndex = index
        selectedSourceName = sourceModel.get( index ).name
        selectedSourceExportEnabled = sourceModel.get( index ).exportEnabled
        sourceInspectorOpen = true
    }

    function closeSourceInspector() {
        hideHelp()
        sourceInspectorOpen = false
    }


    function setSelectedSourceExportEnabled( enabled ) {
        if ( selectedSourceIndex < 0 ) {
            return
        }

        selectedSourceExportEnabled = enabled
        sourceModel.setProperty( selectedSourceIndex, "exportEnabled", enabled )
    }

    Item {
        id: overlayLayer
        anchors.fill: parent
        z: 10000
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
            contentHeight: 56

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
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                spacing: 12

                Label {
                    text: "vcstream"
                    font.pixelSize: 18
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

                VcToolButton {
                    id: preferencesButton
                    objectName: "preferencesButton"
                    theme: root.theme
                    tone: "neutral"
                    hoverEnabled: true

                    text: "Preferences"
                    icon.name: "preferences-system"
                    display: ( appSupervisor && appSupervisor.themeIconAvailable( icon.name ) ? AbstractButton.IconOnly : AbstractButton.TextOnly )

                    onHoveredChanged: {
                        if ( hovered ) {
                            root.showHelp( preferencesButton, "Open preferences." )
                        } else {
                            root.hideHelp()
                        }
                    }

                    onClicked: {
                        root.hideHelp()
                        root.preferencesRequested()
                    }
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Item {
                SplitView.preferredWidth: 260
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellLeftPaneMinWidth : 220 )

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    VcPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        theme: root.theme
                        accentRole: "secondary"

                        ColumnLayout {
                            width: parent.width
                            spacing: 8

                            Label {
                                text: "Participants"
                                font.pixelSize: 14
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

                    VcPanel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
                        theme: root.theme
                        accentRole: "tertiary"

                        ColumnLayout {
                            width: parent.width
                            spacing: 8

                            Label {
                                text: "Sources"
                                font.pixelSize: 14
                                color: ( theme ? theme.textColour : pal.text )
                            }

                            ListModel {
                                id: sourceModel

                                ListElement { name: "Camera"; exportEnabled: false }
                                ListElement { name: "Microphone"; exportEnabled: false }
                                ListElement { name: "Screen share"; exportEnabled: false }
                                ListElement { name: "Music"; exportEnabled: false }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 6
                                color: ( theme ? theme.panelInsetColour : tintSoft )

                                ListView {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    clip: true
                                    model: sourceModel
                                    spacing: 6

                                    delegate: Rectangle {
                                        width: ListView.view.width
                                        height: 34
                                        radius: 6
                                        color: tintSofter

                                        objectName: "sourceRow_" + index

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 10

                                            Label {
                                                text: name
                                                Layout.fillWidth: true
                                                elide: Text.ElideRight
                                            }

                                            Rectangle {
                                                Layout.preferredWidth: 10
                                                Layout.preferredHeight: 10
                                                radius: 5
                                                visible: exportEnabled
                                                color: pal.highlight
                                                opacity: 0.9
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                root.openSourceInspector( index )
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

            Item {
                SplitView.preferredWidth: 520
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellCentrePaneMinWidth : 320 )

                VcPanel {
                    anchors.fill: parent
                    anchors.margins: 10
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
                                font.pixelSize: 18
                                color: ( theme ? theme.metaTextColour : pal.mid )
                            }
                        }

                        Item {
                            id: sourceInspectorOverlay
                            objectName: "sourceInspectorOverlay"
                            anchors.fill: parent
                            visible: root.sourceInspectorOpen
                            z: 100

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
                                anchors.centerIn: parent
                                width: Math.min( parent.width * 0.92, 560 )
                                height: Math.min( parent.height * 0.85, 420 )
                                theme: root.theme
                                accentRole: "secondary"

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.AllButtons
                                    onPressed: function( mouse ) {
                                        mouse.accepted = true
                                    }
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 12

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Label {
                                            text: ( root.selectedSourceName.length > 0 ? root.selectedSourceName : "Source" )
                                            font.pixelSize: 18
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
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

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: 8
                                        color: ( theme ? theme.panelInsetColour : tintSoft )

                                        Label {
                                            anchors.centerIn: parent
                                            text: "Preview (placeholder)"
                                            opacity: 0.75
                                        }
                                    }

                                    VcPanel {
                                        Layout.fillWidth: true
                                        theme: root.theme
                                        accentRole: "tertiary"

                                        ColumnLayout {
                                            width: parent.width
                                            spacing: 6

                                            Label {
                                                text: "Details"
                                                font.pixelSize: 14
                                            }

                                            Label {
                                                text: "Origin: (placeholder)"
                                                opacity: 0.75
                                                wrapMode: Text.Wrap
                                            }

                                            Label {
                                                text: "Owner: (placeholder)"
                                                opacity: 0.75
                                                wrapMode: Text.Wrap
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
                }
            }

            Item {
                SplitView.preferredWidth: 320
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellRightPaneMinWidth : 260 )

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

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
                                width: parent.width
                                spacing: 8

                                Label {
                                    text: "Chat"
                                    font.pixelSize: 14
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
                                width: parent.width
                                spacing: 8

                                Label {
                                    text: "Diagnostics"
                                    font.pixelSize: 14
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
            contentHeight: 30

            background: Rectangle {
                color: ( theme ? theme.panelColour : pal.base )
                border.color: ( theme ? theme.frameColour : pal.mid )
                border.width: 1
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

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
