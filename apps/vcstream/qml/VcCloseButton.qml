import QtQuick 2.15

VcToolButton {
    id: control

    objectName: ""
    tone: "neutral"

    text: ""
    implicitWidth: implicitHeight
    leftPadding: 6
    rightPadding: 6
    topPadding: 6
    bottomPadding: 6

    Accessible.name: "Close"

    contentItem: Item {
        anchors.fill: parent

        Item {
            anchors.centerIn: parent
            width: 18
            height: 18

            readonly property color ink: ( control.theme ? control.theme.textColour : control.palette.text )

            Rectangle {
                anchors.centerIn: parent
                width: 2
                height: 18
                radius: 1
                color: parent.ink
                rotation: 45
                transformOrigin: Item.Center
                antialiasing: true
                property bool vcInkStroke: true
            }

            Rectangle {
                anchors.centerIn: parent
                width: 2
                height: 18
                radius: 1
                color: parent.ink
                rotation: -45
                transformOrigin: Item.Center
                antialiasing: true
                property bool vcInkStroke: true
            }
        }
    }
}
