import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    width: 960
    height: 540
    visible: true
    title: "vcstream"

    header: ToolBar {
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

            CheckBox {
                text: "Connect"
            }

            CheckBox {
                text: "Host Relay"
            }

            CheckBox {
                text: "OBS Bridge"
            }
        }
    }

    footer: ToolBar {
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
                text: "Shell only"
                opacity: 0.75
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Pane {
            SplitView.preferredWidth: 260
            SplitView.minimumWidth: 220
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
                            color: "#000000"
                            opacity: 0.08

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
                    Layout.preferredHeight: 170

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Label {
                            text: "Sources"
                            font.pixelSize: 14
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 6
                            color: "#000000"
                            opacity: 0.08

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

        Pane {
            SplitView.preferredWidth: 520
            SplitView.minimumWidth: 320
            padding: 10

            Frame {
                anchors.fill: parent

                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: "#000000"
                    opacity: 0.06

                    Label {
                        anchors.centerIn: parent
                        text: "Stage / Video Area"
                        font.pixelSize: 18
                        opacity: 0.75
                    }
                }
            }
        }

        Pane {
            SplitView.preferredWidth: 320
            SplitView.minimumWidth: 260
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
                                color: "#000000"
                                opacity: 0.08

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
                                color: "#000000"
                                opacity: 0.08

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
}
