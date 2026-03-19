import QtQuick 2.15
import QtQuick.Controls 2.15

ToolButton {
    id: control

    property var theme
    property string tone: "neutral"

    implicitHeight: ( theme ? theme.compactControlHeight : 34 )
    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6
    spacing: 8
    hoverEnabled: true

    function blendSafe( a, b, t ) {
        if ( theme && theme.blend ) {
            return theme.blend( a, b, t )
        }

        var tt = Math.max( 0.0, Math.min( 1.0, t ) )
        return Qt.rgba(
            a.r * ( 1.0 - tt ) + b.r * tt,
            a.g * ( 1.0 - tt ) + b.g * tt,
            a.b * ( 1.0 - tt ) + b.b * tt,
            1.0 )
    }

    readonly property color fillColour: {
        if ( !theme ) {
            return palette.button
        }
        if ( tone === "primary" ) {
            return theme.primaryAccentColour
        }
        if ( tone === "secondary" ) {
            return theme.secondaryAccentColour
        }
        return theme.panelInsetColour
    }

    readonly property color frameColour: {
        if ( !theme ) {
            return palette.mid
        }
        if ( tone === "neutral" ) {
            return theme.frameColour
        }
        return blendSafe( fillColour, theme.shadowColour, 0.24 )
    }

    readonly property color textRoleColour: {
        if ( !theme ) {
            return palette.buttonText
        }
        if ( tone === "neutral" ) {
            return theme.textColour
        }
        return theme.highlightedTextColour
    }

    readonly property color currentFill: {
        if ( !enabled && theme ) {
            return blendSafe( fillColour, theme.panelColour, 0.55 )
        }
        if ( down && theme ) {
            return blendSafe( fillColour, theme.shadowColour, 0.16 )
        }
        if ( hovered && theme ) {
            return blendSafe( fillColour, theme.panelColour, 0.18 )
        }
        return fillColour
    }

    palette.buttonText: textRoleColour
    palette.text: textRoleColour

    background: Item {
        Rectangle {
            anchors.fill: parent
            radius: ( theme ? theme.controlRadius : 8 )
            color: control.currentFill
            border.color: control.frameColour
            border.width: ( theme ? theme.lineMed : 2 )
        }

        Rectangle {
            visible: control.enabled
            anchors.fill: parent
            anchors.margins: ( theme ? theme.insetGap : 3 )
            radius: Math.max( 0, ( theme ? theme.controlRadius : 8 ) - 2 )
            color: "transparent"
            border.color: ( theme ? theme.frameInnerColour : control.frameColour )
            border.width: ( theme ? theme.lineThin : 1 )
        }

        Rectangle {
            visible: control.visualFocus
            anchors.fill: parent
            anchors.margins: -3
            radius: ( theme ? theme.controlRadius + 3 : 11 )
            color: "transparent"
            border.color: ( theme ? theme.focusRingColour : palette.highlight )
            border.width: 2
        }
    }
}
