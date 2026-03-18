import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    width: 960
    height: 540
    minimumWidth: uiMetrics.minimumWindowWidth
    minimumHeight: uiMetrics.minimumWindowHeight
    visible: true
    title: "vcstream"

    onWidthChanged: {
        if ( width < minimumWidth ) {
            width = minimumWidth
        }
    }

    onHeightChanged: {
        if ( height < minimumHeight ) {
            height = minimumHeight
        }
    }

    QtObject {
        id: uiMetrics
        objectName: "uiMetrics"

        readonly property int landingPageMinWidth: 360
        readonly property int landingPageMinHeight: 420

        readonly property int shellLeftPaneMinWidth: 220
        readonly property int shellCentrePaneMinWidth: 320
        readonly property int shellRightPaneMinWidth: 260
        readonly property int shellPageMinWidth: shellLeftPaneMinWidth + shellCentrePaneMinWidth + shellRightPaneMinWidth
        readonly property int shellPageMinHeight: 540

        readonly property int minimumWindowWidth: Math.max( landingPageMinWidth, shellPageMinWidth )
        readonly property int minimumWindowHeight: Math.max( landingPageMinHeight, shellPageMinHeight )
    }

    function goToLanding() {
        stackView.clear()
        stackView.push( landingComponent )
    }

    function goToShellForJoin() {
        if ( appSupervisor ) {
            appSupervisor.joinRoomEnabled = true
            appSupervisor.hostRoomEnabled = false
        }

        stackView.clear()
        stackView.push( shellComponent )
    }

    function goToShellForHost() {
        if ( appSupervisor ) {
            appSupervisor.hostRoomEnabled = true
            appSupervisor.joinRoomEnabled = false
        }

        stackView.clear()
        stackView.push( shellComponent )
    }

    Component {
        id: landingComponent

        LandingPage {
            uiMetrics: uiMetrics
            onJoinRequested: {
                root.goToShellForJoin()
            }
            onHostRequested: {
                root.goToShellForHost()
            }
        }
    }

    Component {
        id: shellComponent

        ShellPage {
            uiMetrics: uiMetrics
            onDisconnectRequested: {
                if ( appSupervisor ) {
                    appSupervisor.joinRoomEnabled = false
                    appSupervisor.hostRoomEnabled = false
                }

                root.goToLanding()
            }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: landingComponent
    }
}
