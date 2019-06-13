import QtQuick 2.1
import Sailfish.Silica 1.0
import Nemo.DBus 2.0
import com.jolla.settings 1.0
import Nemo.Notifications 1.0
import org.nemomobile.lipstick 0.1 as Lipstick

SettingsToggle {
    id: root

    property int serviceState
    property bool startedFromToggle

    Timer {
        id: repeatedGetState
        interval: 1000
        repeat: false
        onTriggered: {
            dbus.typedCall("GetState", [], function(initialState) {
                serviceState = initialState
            }, function() {
                repeatedGetState.start()
            })
        }
    }

    DBusInterface {
        id: dbus
        service: "org.coderus.screenrecorder"
        path: "/org/coderus/screenrecorder"
        iface: "org.coderus.screenrecorder"
        bus: DBus.SystemBus

        signalsEnabled: true

        function stateChanged(newState) {
            if (repeatedGetState.running) {
                repeatedGetState.stop()
            }

            serviceState = newState
        }

        function recordingFinished(filename) {
            if (startedFromToggle) {
                notification.body = filename
                notification.publish()

                startedFromToggle = false
            }
        }

        Component.onDestruction: {
            call("Quit")
        }

        Component.onCompleted: {
            repeatedGetState.start()
        }
    }

    Lipstick.LauncherItem {
        id: settingsLauncher

        filePath: "/usr/share/applications/screenrecorder-gui.desktop"
    }


    name: "Record screen"
    icon.source: "image://theme/icon-m-screenrecorder-toggle"
    checked: active
    active: serviceState > 1
    busy: serviceState > 2
    activeText: serviceState == 2 ? "Recording..."
                                  : "Saving..."

    onToggled: {
        if (serviceState == 1) {
            dbus.call("Start")
            startedFromToggle = true
        } else if (serviceState == 2) {
            dbus.call("Stop")
        }
    }

    menu: ContextMenu {
        SettingsMenuItem {
            onClicked: settingsLauncher.launchApplication()
        }
    }

    Notification {
        id: notification
        icon: "image://theme/icon-m-screenrecorder-toggle"
        appIcon: "image://theme/screenrecorder-gui"
        appName: "Screenrecorder"
        category: "x-nemo.general.warning"
        previewBody: body
        previewSummary: summary
        summary: "Recording completed"
    }
}
