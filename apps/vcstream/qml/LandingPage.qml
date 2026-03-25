import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: root
    objectName: "landingPage"
    padding: 0

    property var uiMetrics
    property var appPalette
    property var theme

    SystemPalette { id: sysPal }
    readonly property var pal: ( appPalette ? appPalette : sysPal )

    readonly property color windowColour: ( theme ? theme.windowColour : pal.window )
    readonly property color textColour: ( theme ? theme.textColour : pal.text )
    readonly property color accentColour: ( theme ? theme.accentColour : pal.highlight )
    readonly property color accentTextColour: ( theme ? theme.highlightedTextColour : pal.highlightedText )
    readonly property color resolvedHighlightColour: palette.highlight
    readonly property color resolvedButtonColour: palette.button

    palette.window: windowColour
    palette.windowText: textColour
    palette.base: ( theme ? theme.baseColour : pal.base )
    palette.alternateBase: ( theme ? theme.alternateBaseColour : pal.alternateBase )
    palette.text: textColour
    palette.button: ( theme ? theme.buttonColour : pal.button )
    palette.buttonText: textColour
    palette.placeholderText: ( theme ? theme.placeholderTextColour : pal.placeholderText )
    palette.light: ( theme ? theme.lightColour : pal.light )
    palette.midlight: ( theme ? theme.midlightColour : pal.midlight )
    palette.dark: ( theme ? theme.darkColour : pal.dark )
    palette.mid: ( theme ? theme.midColour : pal.mid )
    palette.shadow: ( theme ? theme.shadowColour : pal.shadow )
    palette.highlight: accentColour
    palette.highlightedText: accentTextColour
    palette.link: accentColour
    palette.linkVisited: ( theme ? theme.linkVisitedColour : pal.linkVisited )
    palette.toolTipBase: ( theme ? theme.toolTipBaseColour : pal.toolTipBase )
    palette.toolTipText: ( theme ? theme.toolTipTextColour : pal.toolTipText )

    background: Rectangle {
        color: ( theme ? theme.paperColour : windowColour )
    }

    signal joinRequested()
    signal hostRequested()
    signal preferencesRequested()
    signal hoverHelpRequested( Item target, string text )
    signal hoverHelpHideRequested()

    VcToolButton {
        id: preferencesButton
        objectName: "landingPreferencesButton"
        theme: root.theme
        tone: "neutral"

        hoverEnabled: true

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: ( theme ? theme.spaceTight : 10 )
        z: 1000

        text: "Preferences"
        icon.name: "preferences-system"
        display: ( appSupervisor && appSupervisor.themeIconAvailable( icon.name ) ? AbstractButton.IconOnly : AbstractButton.TextOnly )

        onHoveredChanged: {
            if ( hovered ) {
                root.hoverHelpRequested( preferencesButton, "Open preferences." )
            } else {
                root.hoverHelpHideRequested()
            }
        }

        onClicked: {
            root.preferencesRequested()
        }
    }

    VcPanel {
        anchors.centerIn: parent
        width: Math.min( parent.width * 0.85, ( theme && theme.macroPx ? theme.macroPx( 520 ) : 520 ) )
        theme: root.theme
        accentRole: "tertiary"

        ColumnLayout {
            width: parent.width
            spacing: ( theme ? theme.spaceBase : 16 )

            Label {
                text: "vcstream"
                font.pixelSize: ( theme ? theme.fontTitlePx : 28 )
                color: textColour
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }

            Label {
                text: "Choose how you want to start."
                color: ( theme ? theme.metaTextColour : textColour )
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }

            VcButton {
                id: joinRoomButton
                objectName: "joinRoomButton"
                theme: root.theme
                tone: "primary"
                text: "Join room"
                Layout.fillWidth: true
                Layout.preferredHeight: Math.round( ( theme ? theme.controlHeight : 42 ) * 1.5 )
                font.pixelSize: ( theme ? theme.fontHeadingPx : 18 )

                onClicked: {
                    root.joinRequested()
                }

            }

            VcButton {
                id: hostRoomButton
                objectName: "hostRoomButton"
                theme: root.theme
                tone: "secondary"
                text: "Host room"
                Layout.fillWidth: true
                Layout.preferredHeight: Math.round( ( theme ? theme.controlHeight : 42 ) * 1.5 )
                font.pixelSize: ( theme ? theme.fontHeadingPx : 18 )

                onClicked: {
                    root.hostRequested()
                }

            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 4
            }

            Label {
                text: "You can change this later."
                color: ( theme ? theme.metaTextColour : textColour )
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
        }
    }
}
