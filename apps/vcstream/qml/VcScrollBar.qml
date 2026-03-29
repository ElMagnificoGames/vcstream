import QtQuick 2.15
import QtQuick.Controls 2.15

ScrollBar {
    id: control

    property var theme
    property bool vcStyled: true

    implicitWidth: 12
    implicitHeight: 12
    minimumSize: 0.12

    // Prevent the thumb from painting outside the track during the first
    // layout tick (some popups briefly report zero height).
    clip: true

    // Some desktop styles add internal padding that can collapse the available
    // track size for narrow bars (e.g. availableWidth becomes 0). We want the
    // thumb to remain visible and use the full allocated geometry.
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // Render whenever the content is scrollable.
    // This avoids style/plugin differences that can hide scroll bars entirely.
    readonly property real vcSaneSize: ( isFinite( control.size ) && control.size > 0.0 ) ? control.size : 0.0
    readonly property real vcEffectiveSize: Math.max( control.minimumSize, vcSaneSize )

    readonly property bool vcCanScroll: ( vcSaneSize < 0.999 )

    // Avoid a 1-frame "dot" when geometry is not yet settled.
    // Some legitimate scroll regions are short (for example, compact lists), so
    // keep the threshold low.
    readonly property bool vcGeometryReady: ( control.orientation === Qt.Horizontal )
        ? ( control.height >= 8 ) && ( control.width >= 20 )
        : ( control.width >= 8 ) && ( control.height >= 20 )
    visible: vcGeometryReady && ( control.policy !== ScrollBar.AlwaysOff ) && vcCanScroll
    opacity: 1.0

    background: Rectangle {
        radius: 6
        anchors.fill: parent
        color: ( theme ? Qt.rgba( theme.panelInsetColour.r, theme.panelInsetColour.g, theme.panelInsetColour.b, 0.35 ) : "transparent" )
        border.color: ( theme ? Qt.rgba( theme.frameColour.r, theme.frameColour.g, theme.frameColour.b, 0.40 ) : "transparent" )
        border.width: ( theme ? 1 : 0 )
    }

    contentItem: Rectangle {
        radius: 6
        color: ( theme ? theme.primaryAccentColour : palette.highlight )
        opacity: control.pressed ? 0.95 : ( control.hovered ? 0.90 : 0.80 )

        border.color: ( theme ? Qt.rgba( theme.frameColour.r, theme.frameColour.g, theme.frameColour.b, 0.55 ) : "transparent" )
        border.width: ( theme ? 1 : 0 )

        readonly property real trackW: control.width
        readonly property real trackH: control.height

        width: ( control.orientation === Qt.Horizontal )
            ? Math.max( 6, trackW * vcEffectiveSize )
            : trackW

        height: ( control.orientation === Qt.Horizontal )
            ? trackH
            : Math.max( 6, trackH * vcEffectiveSize )

        x: ( control.orientation === Qt.Horizontal )
            ? Math.max( 0, Math.min( trackW - width, trackW * control.position ) )
            : 0

        y: ( control.orientation === Qt.Horizontal )
            ? 0
            : Math.max( 0, Math.min( trackH - height, trackH * control.position ) )
    }
}
