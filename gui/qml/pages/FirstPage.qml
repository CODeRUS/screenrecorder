import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.DBus 2.0
import Nemo.Configuration 1.0

Page {
    id: page

    Timer {
        interval: 1000
        repeat: false
        running: true
        onTriggered: {
            var initialState = dbus.typedCall("GetState", [], function(initialState) {
                console.log("Initial state:", initialState)
                serviceState = initialState
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
            console.log(newState)
            serviceState = newState
            if (serviceState == 2) {
                filenameLabel.text = ""
            }
        }

        function recordingFinished(filename) {
            console.log(filename)
            filenameLabel.text = filename
        }

        Component.onDestruction: {
            dbus.call("Quit")
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
                label: qsTr("JPEG frames quality")
                valueText: qsTr("%1%").arg(value)
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
                    }
                }
                enabled: serviceState > 0 && serviceState < 3
                onClicked: {
                    trigger()
                }
                function trigger() {
                    if (serviceState == 1) {
                        dbus.call("Start")
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
}
