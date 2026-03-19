import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: control

    property var theme
    property string tone: "neutral"
    property bool compact: false

    implicitHeight: compact ? ( theme ? theme.compactControlHeight : 34 ) : ( theme ? theme.controlHeight : 42 )
    leftPadding: compact ? 10 : 14
    rightPadding: compact ? 10 : 14
    topPadding: compact ? 6 : 9
    bottomPadding: compact ? 6 : 9
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

    function srgbToLinear01( u ) {
        if ( u <= 0.04045 ) {
            return u / 12.92
        }
        return Math.pow( ( u + 0.055 ) / 1.055, 2.4 )
    }

    function relativeLuminance( c ) {
        const r = srgbToLinear01( c.r )
        const g = srgbToLinear01( c.g )
        const b = srgbToLinear01( c.b )
        return 0.2126 * r + 0.7152 * g + 0.0722 * b
    }

    function contrastingTextForFill( fill ) {
        const lum = relativeLuminance( fill )

        // Pick whichever of black/white yields higher contrast against the fill.
        const contrastWithBlack = ( lum + 0.05 ) / 0.05
        const contrastWithWhite = 1.05 / ( lum + 0.05 )

        const preferBlack = contrastWithBlack >= contrastWithWhite

        if ( theme ) {
            return ( preferBlack ? theme.anchorBlack : theme.anchorWhite )
        }

        return ( preferBlack ? "black" : "white" )
    }

    readonly property color baseFill: {
        if ( !theme ) {
            return palette.button
        }
        if ( tone === "primary" ) {
            return theme.primaryAccentFillColour
        }
        if ( tone === "secondary" ) {
            return theme.secondaryAccentFillColour
        }
        if ( tone === "tertiary" ) {
            return theme.tertiaryAccentFillColour
        }
        if ( tone === "danger" ) {
            return theme.dangerColour
        }
        return theme.panelColour
    }

    readonly property color baseFrame: {
        if ( !theme ) {
            return palette.mid
        }
        if ( tone === "neutral" ) {
            return theme.frameColour
        }

        // Accent-toned buttons should not get near-black frames on light surfaces.
        if ( theme.panelSurfaceIsDark ) {
            return blendSafe( baseFill, theme.shadowColour, 0.28 )
        }

        return blendSafe( baseFill, theme.textColour, 0.18 )
    }

    readonly property color textRoleColour: {
        if ( !theme ) {
            return palette.buttonText
        }
        if ( tone === "neutral" || tone === "tertiary" ) {
            return theme.textColour
        }
        return contrastingTextForFill( currentFill )
    }

    readonly property color currentFill: {
        if ( !enabled && theme ) {
            return blendSafe( baseFill, theme.panelColour, 0.55 )
        }
        if ( down && theme ) {
            return blendSafe( baseFill, theme.shadowColour, 0.18 )
        }
        if ( hovered && theme ) {
            return blendSafe( baseFill, theme.panelInsetColour, 0.22 )
        }
        return baseFill
    }

    // Exposed so tests (and theme consumers) can reason about text contrast.
    readonly property color surfaceColour: currentFill

    palette.buttonText: textRoleColour
    palette.text: textRoleColour

    contentItem: Item {
        id: contentRoot

        readonly property bool showIcon: ( control.display !== AbstractButton.TextOnly )
            && ( ( control.icon && control.icon.name && control.icon.name.length > 0 )
                || ( control.icon && control.icon.source && control.icon.source.toString().length > 0 ) )

        readonly property bool showText: ( control.display !== AbstractButton.IconOnly )
            && control.text && control.text.length > 0

        readonly property url resolvedIconSource: {
            if ( control.icon && control.icon.source && control.icon.source.toString().length > 0 ) {
                return control.icon.source
            }
            if ( control.icon && control.icon.name && control.icon.name.length > 0 ) {
                return "image://theme/" + control.icon.name
            }
            return ""
        }

        implicitWidth: contentRow.implicitWidth
        implicitHeight: contentRow.implicitHeight

        Row {
            id: contentRow
            spacing: control.spacing
            anchors.centerIn: parent

            Image {
                visible: contentRoot.showIcon
                source: contentRoot.resolvedIconSource
                width: 20
                height: 20
                sourceSize.width: 40
                sourceSize.height: 40
                fillMode: Image.PreserveAspectFit
                opacity: control.enabled ? 1.0 : 0.55
            }

            Text {
                visible: contentRoot.showText
                text: control.text
                font: control.font
                color: control.textRoleColour
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                opacity: control.enabled ? 1.0 : 0.55
            }
        }
    }

    background: Item {
        Rectangle {
            anchors.fill: parent
            radius: ( theme ? theme.controlRadius : 8 )
            color: control.currentFill
            border.color: control.baseFrame
            border.width: ( theme ? theme.lineMed : 2 )
        }

        Rectangle {
            visible: control.enabled
            anchors.fill: parent
            anchors.margins: ( theme ? theme.insetGap : 3 )
            radius: Math.max( 0, ( theme ? theme.controlRadius : 8 ) - 2 )
            color: "transparent"
            border.color: ( theme ? theme.frameInnerColour : control.baseFrame )
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
