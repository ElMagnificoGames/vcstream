import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    width: 960
    height: 540
    visible: true
    title: "vcstream"

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
