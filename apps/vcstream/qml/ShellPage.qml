import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    objectName: "shellPage"

    property var uiMetrics

    SystemPalette {
        id: pal
    }

    readonly property color tintSoft: Qt.rgba( pal.text.r, pal.text.g, pal.text.b, 0.06 )
    readonly property color tintSofter: Qt.rgba( pal.text.r, pal.text.g, pal.text.b, 0.04 )
    readonly property color scrimColour: Qt.rgba( pal.shadow.r, pal.shadow.g, pal.shadow.b, 0.35 )

    signal disconnectRequested()

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
            contentHeight: 44

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12

                Label {
                    text: "vcstream"
                    font.pixelSize: 18
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: disconnectButton
                    objectName: "disconnectButton"
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
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Pane {
                SplitView.preferredWidth: 260
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellLeftPaneMinWidth : 220 )
                padding: 10

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    Frame {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            Label {
                                text: "Participants"
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 6
                                color: tintSoft

                                Label {
                                    anchors.centerIn: parent
                                    text: "(placeholder)"
                                    opacity: 0.65
                                }
                            }
                        }
                    }

                    Frame {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            Label {
                                text: "Sources"
                                font.pixelSize: 14
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
                                color: tintSoft

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
                                opacity: 0.65
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            Pane {
                SplitView.preferredWidth: 520
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellCentrePaneMinWidth : 320 )
                padding: 10

                Frame {
                    anchors.fill: parent

                    Item {
                        id: stageRoot
                        objectName: "stageRoot"
                        anchors.fill: parent

                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: tintSofter

                            Label {
                                anchors.centerIn: parent
                                text: "Stage / Participant Grid"
                                font.pixelSize: 18
                                opacity: 0.75
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

                            Rectangle {
                                id: sourceInspectorPanel
                                objectName: "sourceInspectorPanel"
                                anchors.centerIn: parent
                                width: Math.min( parent.width * 0.92, 560 )
                                height: Math.min( parent.height * 0.85, 420 )
                                radius: 10
                                color: pal.base
                                border.color: pal.mid
                                border.width: 1

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.AllButtons
                                    onPressed: function( mouse ) {
                                        mouse.accepted = true
                                    }
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 12

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Label {
                                            text: ( root.selectedSourceName.length > 0 ? root.selectedSourceName : "Source" )
                                            font.pixelSize: 18
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }

                                        ToolButton {
                                            id: closeInspectorButton
                                            objectName: "sourceInspectorCloseButton"
                                            text: "X"
                                            onClicked: {
                                                root.closeSourceInspector()
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        radius: 8
                                        color: tintSoft

                                        Label {
                                            anchors.centerIn: parent
                                            text: "Preview (placeholder)"
                                            opacity: 0.75
                                        }
                                    }

                                    Frame {
                                        Layout.fillWidth: true

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 10
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

                                        CheckBox {
                                            id: inspectorExportToggle
                                            objectName: "sourceInspectorBrowserExportToggle"
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

            Pane {
                SplitView.preferredWidth: 320
                SplitView.minimumWidth: ( uiMetrics ? uiMetrics.shellRightPaneMinWidth : 260 )
                padding: 10

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    TabBar {
                        id: rightTabs
                        Layout.fillWidth: true

                        TabButton { text: "Chat" }
                        TabButton { text: "Diagnostics" }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: rightTabs.currentIndex

                        Frame {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 8

                                Label {
                                    text: "Chat"
                                    font.pixelSize: 14
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6
                                    color: tintSoft

                                    Label {
                                        anchors.centerIn: parent
                                        text: "(placeholder)"
                                        opacity: 0.65
                                    }
                                }

                                TextField {
                                    Layout.fillWidth: true
                                    placeholderText: "Type a message (placeholder)"
                                    enabled: false
                                }
                            }
                        }

                        Frame {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 8

                                Label {
                                    text: "Diagnostics"
                                    font.pixelSize: 14
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6
                                    color: tintSoft

                                    Label {
                                        anchors.centerIn: parent
                                        text: "(placeholder)"
                                        opacity: 0.65
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

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Label {
                    text: ( appSupervisor ? ( "Version " + appSupervisor.appVersion ) : "" )
                    opacity: 0.75
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
                    opacity: 0.75
                }
            }
        }
    }
}
