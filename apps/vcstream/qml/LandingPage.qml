import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    objectName: "landingPage"

    SystemPalette {
        id: pal
    }

    signal joinRequested()
    signal hostRequested()

    Rectangle {
        anchors.fill: parent
        color: pal.window
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min( parent.width * 0.85, 520 )
        spacing: 16

        Label {
            text: "vcstream"
            font.pixelSize: 28
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Label {
            text: "Choose how you want to start."
            opacity: 0.75
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Button {
            objectName: "joinRoomButton"
            text: "Join room"
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            font.pixelSize: 18

            onClicked: {
                root.joinRequested()
            }
        }

        Button {
            objectName: "hostRoomButton"
            text: "Host room"
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            font.pixelSize: 18

            onClicked: {
                root.hostRequested()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 8
        }

        Label {
            text: "You can change this later."
            opacity: 0.65
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
    }
}
