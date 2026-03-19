import QtQuick 2.15

Item {
    id: root

    property var theme
    property string accentRole: "none"
    property bool inset: false

    property int padding: ( theme ? theme.panelPadding : 12 )
    property int insetGap: ( theme ? theme.insetGap : 3 )
    property int radius: ( theme ? theme.panelRadius : 8 )
    property int lineThin: ( theme ? theme.lineThin : 1 )
    property int lineMed: ( theme ? theme.lineMed : 2 )

    default property alias content: contentHost.data

    readonly property color surfaceColour: {
        if ( !theme ) {
            return "transparent"
        }
        return inset ? theme.panelInsetColour : theme.panelColour
    }

    readonly property color frameColour: ( theme ? theme.frameColour : "#666666" )
    readonly property color innerFrameColour: ( theme ? theme.frameInnerColour : "#888888" )
    readonly property color accentColour: {
        if ( !theme ) {
            return "transparent"
        }
        if ( accentRole === "primary" ) return theme.railPrimaryColour
        if ( accentRole === "secondary" ) return theme.railSecondaryColour
        if ( accentRole === "tertiary" ) return theme.railTertiaryColour
        if ( accentRole === "danger" ) return theme.railDangerColour
        return "transparent"
    }

    implicitWidth: padding * 2 + contentHost.implicitWidth
    implicitHeight: padding * 2 + contentHost.implicitHeight

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: ( theme ? root.surfaceColour : "transparent" )
        border.color: root.frameColour
        border.width: root.lineMed
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: root.insetGap
        radius: Math.max( 0, root.radius - 2 )
        color: "transparent"
        border.color: root.innerFrameColour
        border.width: root.lineThin
    }

    Rectangle {
        visible: ( root.accentRole === "primary" || root.accentRole === "secondary" || root.accentRole === "danger" )
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 6
        radius: root.radius
        color: root.accentColour
    }

    Item {
        id: contentHost
        anchors.fill: parent
        anchors.margins: root.padding
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
    }
}
