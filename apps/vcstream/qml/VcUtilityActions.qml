import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Row {
    id: root

    property var theme
    property bool showScheduling: true
    property bool showSupport: true
    property bool showPreferences: true

    property string schedulingObjectName: ""
    property string supportObjectName: ""
    property string preferencesObjectName: ""

    signal schedulingRequested()
    signal supportRequested()
    signal preferencesRequested()
    signal hoverHelpRequested( Item target, string text )
    signal hoverHelpHideRequested()

    spacing: ( theme ? theme.spaceCompact : 8 )

    VcButton {
        id: schedulingButton
        objectName: root.schedulingObjectName
        visible: root.showScheduling
        theme: root.theme
        tone: "neutral"
        compact: true
        text: "Scheduling"

        // Avoid opening overlays during the automatic interaction sweep.
        property bool testSkipActivate: true

        onClicked: root.schedulingRequested()
    }

    VcButton {
        id: supportButton
        objectName: root.supportObjectName
        visible: root.showSupport
        theme: root.theme
        tone: "neutral"
        compact: true
        text: "Support"

        // Avoid opening overlays during the automatic interaction sweep.
        property bool testSkipActivate: true

        onClicked: root.supportRequested()
    }

    VcToolButton {
        id: preferencesButton
        objectName: root.preferencesObjectName
        visible: root.showPreferences
        theme: root.theme
        tone: "neutral"
        hoverEnabled: true

        text: "Preferences"
        icon.name: "preferences-system"
        display: ( typeof appSupervisor !== "undefined" && appSupervisor && appSupervisor.themeIconAvailable( icon.name )
            ? AbstractButton.IconOnly
            : AbstractButton.TextOnly )

        onHoveredChanged: {
            if ( hovered ) {
                root.hoverHelpRequested( preferencesButton, "Open preferences." )
            } else {
                root.hoverHelpHideRequested()
            }
        }

        onClicked: root.preferencesRequested()
    }
}
