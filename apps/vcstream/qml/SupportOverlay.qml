import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    objectName: "supportOverlayRoot"

    property bool open: false
    property var theme
    property var appPalette
    property string twitchUrl: ""
    property string socialsUrl: ""

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
        objectName: "supportPanel"
        anchors.centerIn: parent
        width: Math.min( parent.width * 0.92, ( theme && theme.macroPx ? theme.macroPx( 760 ) : 760 ) )
        height: Math.min( parent.height * 0.88, ( theme && theme.macroPx ? theme.macroPx( 620 ) : 620 ) )
        theme: root.theme
        accentRole: "primary"

        ColumnLayout {
            anchors.fill: parent
            spacing: ( root.theme ? root.theme.spaceTight : 12 )

            RowLayout {
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: ( root.theme ? root.theme.spaceNudge : 4 )

                    Label {
                        text: "How to support Robert"
                        font.pixelSize: ( root.theme ? root.theme.fontHeadingPx : 18 )
                        font.family: ( root.theme ? root.theme.bodyFontFamily : Qt.application.font.family )
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: ( root.theme ? root.theme.textColour : root.pal.text )
                    }

                    Label {
                        text: "Free support matters here."
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                    }
                }

                VcCloseButton {
                    objectName: "supportCloseButton"
                    theme: root.theme
                    onClicked: root.closeRequested()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ScrollView {
                    id: supportScrollView
                    objectName: "supportScrollView"
                    anchors.fill: parent
                    clip: true
                    rightPadding: ( supportScrollBar.width + ( root.theme ? root.theme.spaceTight : 12 ) )
                    bottomPadding: ( supportHScrollBar.visible
                        ? ( supportHScrollBar.height + ( root.theme ? root.theme.spaceTight : 12 ) )
                        : 0 )

                    ScrollBar.horizontal: VcScrollBar {
                        id: supportHScrollBar
                        objectName: "supportScrollBarH"
                        theme: root.theme
                        policy: ScrollBar.AsNeeded
                        orientation: Qt.Horizontal
                        width: supportScrollView.width
                        x: 0
                        y: supportScrollView.height - height
                    }

                    ScrollBar.vertical: VcScrollBar {
                        id: supportScrollBar
                        objectName: "supportScrollBar"
                        theme: root.theme
                        policy: ScrollBar.AsNeeded
                        height: supportScrollView.height
                        x: supportScrollView.width - width
                        y: 0
                    }

                    contentItem: Flickable {
                        id: supportFlickable
                        width: supportScrollView.availableWidth
                        height: supportScrollView.availableHeight
                        boundsBehavior: Flickable.StopAtBounds
                        flickableDirection: Flickable.VerticalFlick

                        ScrollIndicator.horizontal: ScrollIndicator { visible: false }
                        ScrollIndicator.vertical: ScrollIndicator { visible: false }

                        contentWidth: width
                        contentHeight: supportColumn.implicitHeight

                        Column {
                            id: supportColumn
                            width: supportFlickable.width
                            spacing: ( root.theme ? root.theme.spaceBase : 16 )

                            Label {
                                objectName: "supportIntroLabel"
                                width: parent.width
                                wrapMode: Text.Wrap
                                lineHeight: 1.2
                                font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                color: ( root.theme ? root.theme.textColour : root.pal.text )
                                text: "I'm trying to grow my Twitch channel because streaming is how I'm trying to build financial independence for the first time in my life. I'm autistic, so finding a path to stable independent work means more to me than I can easily put into words."
                            }

                            Label {
                                width: parent.width
                                wrapMode: Text.Wrap
                                color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                text: "If you'd like to help, these are some of the most useful ways to do that without spending any money:"
                            }

                            Repeater {
                                model: [
                                    {
                                        title: "Share the stream with people who might genuinely enjoy it.",
                                        detail: "Personal recommendations are one of the most effective ways to help a channel grow."
                                    },
                                    {
                                        title: "Mention the channel in relevant places.",
                                        detail: "If it fits naturally in a group, forum, server, or community, that kind of word of mouth can make a real difference."
                                    },
                                    {
                                        title: "Follow on Twitch if you have an account.",
                                        detail: "That makes it easier to come back and helps the channel build a steady audience over time."
                                    },
                                    {
                                        title: "Say hello in chat when you feel like it.",
                                        detail: "A friendly, active chat makes the stream feel welcoming and worth joining."
                                    },
                                    {
                                        title: "Watch live when you can.",
                                        detail: "Regular viewers help the channel build momentum."
                                    },
                                    {
                                        title: "Clip and share moments you enjoy.",
                                        detail: "A short highlight can introduce new people to the channel quickly."
                                    },
                                    {
                                        title: "Keep up with my socials and share updates that seem worth passing on.",
                                        detail: "That helps people discover the channel even when I'm not live."
                                    }
                                ]

                                delegate: VcPanel {
                                    objectName: "supportItem" + index
                                    width: parent.width
                                    theme: root.theme
                                    inset: true
                                    accentRole: ( index < 2 ? "secondary" : "tertiary" )

                                    ColumnLayout {
                                        width: parent.width
                                        spacing: ( root.theme ? root.theme.spaceNudge : 4 )

                                        Label {
                                            Layout.fillWidth: true
                                            wrapMode: Text.Wrap
                                            font.pixelSize: ( root.theme ? root.theme.fontBasePx : 14 )
                                            font.bold: true
                                            color: ( root.theme ? root.theme.textColour : root.pal.text )
                                            text: modelData.title
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            wrapMode: Text.Wrap
                                            color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                            text: modelData.detail
                                        }
                                    }
                                }
                            }

                            Label {
                                objectName: "supportPaidNote"
                                width: parent.width
                                wrapMode: Text.Wrap
                                lineHeight: 1.2
                                font.pixelSize: ( root.theme ? root.theme.fontSmallPx : 12 )
                                color: ( root.theme ? root.theme.metaTextColour : root.pal.mid )
                                text: "If you'd like to support financially, gifting subs to the community is a lovely way to help the channel while also giving something to other viewers."
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ( root.theme ? root.theme.spaceCompact : 8 )

                VcButton {
                    objectName: "supportTwitchButton"
                    theme: root.theme
                    tone: "primary"
                    text: "Open Twitch"
                    Layout.fillWidth: true
                    property bool testSkipActivate: true

                    onClicked: root.openExternalUrl( root.twitchUrl )
                }

                VcButton {
                    objectName: "supportSocialsButton"
                    theme: root.theme
                    tone: "neutral"
                    text: "Contact / socials"
                    Layout.fillWidth: true
                    property bool testSkipActivate: true

                    onClicked: root.openExternalUrl( root.socialsUrl )
                }
            }
        }
    }
}
