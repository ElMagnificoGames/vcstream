import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    objectName: "schedulingOverlayRoot"

    property bool open: false
    property var theme
    property var appPalette
    property string url: ""

    signal closeRequested()

    visible: open
    z: 1200

    SystemPalette { id: sysPal }
    readonly property var pal: ( appPalette ? appPalette : sysPal )

    function openExternalUrl( urlString ) {
        if ( urlString && urlString.length > 0 ) {
            Qt.openUrlExternally( urlString )
        }
    }

    Rectangle {
        anchors.fill: parent
        color: ( theme ? theme.scrimColour : Qt.rgba( pal.shadow.r, pal.shadow.g, pal.shadow.b, 0.35 ) )

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
        objectName: "schedulingPanel"
        anchors.centerIn: parent
        width: Math.min( parent.width * 0.9, ( theme && theme.macroPx ? theme.macroPx( 560 ) : 560 ) )
        theme: root.theme
        accentRole: "secondary"

        ColumnLayout {
            width: parent.width
            spacing: ( root.theme ? root.theme.spaceTight : 12 )

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: "Need help scheduling?"
                    font.pixelSize: ( root.theme ? root.theme.fontHeadingPx : 18 )
                    font.family: ( root.theme ? root.theme.bodyFontFamily : Qt.application.font.family )
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: ( root.theme ? root.theme.textColour : root.pal.text )
                }

                VcCloseButton {
                    objectName: "schedulingCloseButton"
                    theme: root.theme
                    onClicked: root.closeRequested()
                }
            }

            Label {
                text: "If finding a time is the awkward part, I also have a free online scheduling tool that helps people compare availability and settle on a time that works."
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: ( root.theme ? root.theme.textColour : root.pal.text )
            }

            Label {
                text: "Open it in your browser when you're ready."
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                VcButton {
                    objectName: "schedulingOpenButton"
                    theme: root.theme
                    tone: "secondary"
                    text: "Open scheduling page"
                    Layout.fillWidth: true
                    property bool testSkipActivate: true

                    onClicked: root.openExternalUrl( root.url )
                }
            }
        }
    }
}
