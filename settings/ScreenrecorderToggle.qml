import QtQuick 2.1
import Sailfish.Silica 1.0
import Nemo.DBus 2.0
import com.jolla.settings 1.0
import com.jolla.settings.system 1.0
import Nemo.Notifications 1.0
import org.nemomobile.lipstick 0.1 as Lipstick
import Nemo.Configuration 1.0
import QtMultimedia 5.6

SettingsToggle {
    id: root

    property int serviceState
    property bool startedFromToggle
    property int tickCount
/*
    MediaPlayer {
        id: tickPlayer
        source: Qt.resolvedUrl("/usr/share/screenrecorder-gui/qml/sounds/tick.wav")
        playing:false
    }
*/
    // For some reason, neither delay nor tick timer work correctly
    // perhaps the settings and not being read?
 /*
    Timer {
        id: delayTimer
        interval: conf.startdelay > 0 ? conf.startdelay * 1000 : 0
        repeat: false
        running: false
        onTriggered: {
            trigger();
        }
        function trigger() {
            tickTimer.stop()
            dbus.call("Start")

        }
    }
    Timer {
        id: tickTimer
        interval: 1000
        repeat: true
        onTriggered: {
            if (tickCount < conf.startdelay)  tickPlayer.play()
            tickCount++

        }
    }
*/
    ConfigurationGroup {
        id: conf
        path: "/org/coderus/screenrecorder"
        property int fps: 24
        property int buffers: 48
        property bool fullmode: false
        property real scale: 1.0
        property int quality: 100
        property bool smooth: false
        property bool convert: false
        property int startdelay: 1
    }
    DBusInterface {
        id: dbus
        service: "org.coderus.screenrecorder"
        path: "/org/coderus/screenrecorder"
        iface: "org.coderus.screenrecorder"
        bus: DBus.SessionBus

        signalsEnabled: true

        function stateChanged(newState) {
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
            //call("Quit")
        }

        Component.onCompleted: {
            dbus.typedCall("GetState", [], function(initialState) {
                serviceState = initialState
            });
           // delayTimer.interval = conf.startdelay * 1000
        }
    }

    Lipstick.LauncherItem {
        id: settingsLauncher
        filePath: "/usr/share/applications/screenrecorder-gui.desktop"
    }


    name: "Record screen"
    icon.source: "image://theme/icon-m-camera"
    checked: active
    active: serviceState > 1
    busy: serviceState > 2
    activeText: serviceState == 2 ? "Recording..."
                                  : "Saving..."

    onToggled: {
        if (serviceState == 1) {
            startedFromToggle = true
            dbus.call("Start")
            //sadly, timers are borked
            /*
            if (conf.startdelay > 0 ) {
                delayTimer.start()
                tickTimer.start()
            } else {
                dbus.call("Start")
            }*/
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
        icon: "image://theme/icon-m-camera"
        appIcon: "image://theme/screenrecorder-gui"
        appName: "Screenrecorder"
        category: "x-nemo.general.warning"
        previewBody: body
        previewSummary: summary
        summary: "Recording completed"
    }
}
