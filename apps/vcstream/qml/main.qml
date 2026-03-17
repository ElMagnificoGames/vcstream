import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: root
    width: 960
    height: 540
    visible: true
    title: "vcstream"

    Rectangle {
        anchors.fill: parent
        color: "#1f232a"

        Text {
            anchors.centerIn: parent
            text: "vcstream"
            color: "#e6e9ef"
            font.pixelSize: 28
        }
    }
}
