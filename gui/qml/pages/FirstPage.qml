import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.DBus 2.0
import Nemo.Configuration 1.0
import Nemo.Notifications 1.0
import QtMultimedia 5.6

Page {
    id: page
    property bool startedFromToggle:false

    MediaPlayer {
        id: tickPlayer
        source: Qt.resolvedUrl("../sounds/tick.wav")
        autoPlay: false
    }
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
    Timer {
        id: delayTimer
        interval: 1000
        repeat: false
        running:false
        onTriggered: {
            trigger();
        }
        function trigger() {
            if (serviceState == 1) {
                dbus.call("Start")
            }
            delayTimer.stop();
            tickTimer.stop();
        }
    }
    Timer {
        id: tickTimer
        interval: 1000
        repeat: true
        running:false
        onTriggered: {
            tickPlayer.play()
        }
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
            if (repeatedGetState.running) {
                repeatedGetState.stop()
            }
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
        }
    }

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
        property int startdelay: 0
    }

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Screenrecorder")
            }
            Slider {
                id: startDelaySlider
                width: parent.width
                minimumValue: 0
                maximumValue: 10
                stepSize: 1
                value: conf.startdelay
                label: qsTr("Start delay")
                valueText: qsTr("%1 s").arg(value)
                onReleased: {
                    delayTimer.interval = value * 1000
                    conf.startdelay = value
                }
            }
            Slider {
                id: fpsSlider
                width: parent.width
                minimumValue: 1
                maximumValue: 24
                stepSize: 1
                value: conf.fps
                label: qsTr("Frames per second")
                valueText: qsTr("%1 fps").arg(value)
                onReleased: {
                    conf.fps = value
                    conf.buffers = value * 2
                }
            }

            Slider {
                id: buffersSlider
                width: parent.width
                minimumValue: 2
                maximumValue: 60
                stepSize: 1
                value: conf.buffers
                label: qsTr("Buffers to store frames")
                valueText: qsTr("%1").arg(value)
                onReleased: {
                    conf.buffers = value
                }
            }

            TextSwitch {
                id: fullSwitch
                text: qsTr("Store all frames")
                checked: conf.fullmode
                automaticCheck: false
                onClicked: {
                    conf.fullmode = !checked
                }
            }

            Slider {
                id: qualitySlider
                width: parent.width
                minimumValue: 10
                maximumValue: 100
                stepSize: 1
                value: conf.quality
                label: qsTr("Encoding quality")
                valueText: qsTr("%1").arg(value)
                onReleased: {
                    conf.quality = value
                }
            }

            Slider {
                id: scaleSlider
                width: parent.width
                minimumValue: 1
                maximumValue: 100
                stepSize: 1
                value: conf.scale * 100
                label: qsTr("Frames scale")
                valueText: qsTr("%1%").arg(value)
                onReleased: {
                    conf.scale = value / 100
                }
            }

            TextSwitch {
                id: smoothSwitch
                visible: conf.scale < 1.0
                text: qsTr("Apply smooth transformation")
                checked: conf.smooth
                automaticCheck: false
                onClicked: {
                    conf.smooth = !checked
                }
            }

            TextSwitch {
                id: convertSwitch
                text: qsTr("Convert to MP4 (smaller files)")
                checked: conf.convert
                automaticCheck: false
                onClicked: {
                    conf.convert = !checked
                }
            }
            Button {
                id: actionButton
                anchors.horizontalCenter: parent.horizontalCenter
                text: {
                    switch (serviceState) {
                    case 0:
                        return qsTr("Loading")
                    case 1:
                        return qsTr("Record")
                    case 2:
                        return qsTr("Stop")
                    case 3:
                        return qsTr("Saving, please wait")
                    case 4:
                        return qsTr("Converting, please wait")
                    }
                }
                enabled: serviceState > 0 && serviceState < 3
                onClicked: {
                    trigger()
                }
                function trigger() {
                    if (serviceState == 1) {
                        startedFromToggle = true
                        delayTimer.running = true
                        tickTimer.running = true
                    } else if (serviceState == 2) {
                        dbus.call("Stop")
                    }
                }

            }

            Label {
                id: filenameLabel
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.horizontalPageMargin
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                visible: text
            }
        }

        VerticalScrollDecorator {}
    }

    Connections {
        target: appWindow
        onCoverTriggered: {
            console.log("cover")
            if (actionButton.enabled) {
                actionButton.trigger()
            }
        }
    }
    Component.onCompleted: {
        dbus.typedCall("GetState", [], function(initialState) {
            serviceState = initialState
            delayTimer.interval = conf.startdelay * 1000
        });
        if (serviceState == 0) {
            repeatedGetState.start()
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
