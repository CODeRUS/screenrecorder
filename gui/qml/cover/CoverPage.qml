import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    Image {
        id: logo
        source: "../screenrecorder-gui.png"
        anchors.centerIn: parent
    }
    Label {
        id: label
        anchors.centerIn: parent
        text: {
            if ( serviceState == 2 ) return qsTr("Recording...")
            if ( serviceState == 1 ) return qsTr("Recorder ready ...")
            if ( serviceState > 2 ) return qsTr("Saving...")
        }

    }
    CoverActionList {
        id: coverAction
        enabled: serviceState > 0 && serviceState < 3

        CoverAction {
            iconSource: serviceState == 1
                        ? "image://theme/icon-cover-location"
                        : "image://theme/icon-cover-cancel"
            onTriggered: {
                coverTriggered()
            }
        }
    }
}

