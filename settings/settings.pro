TEMPLATE = aux

settingsjson.files = screenrecorder.json
settingsjson.path = /usr/share/jolla-settings/entries

INSTALLS += settingsjson

settingsqml.files = ScreenrecorderToggle.qml
settingsqml.path = /usr/share/jolla-settings/pages/screenrecorder

INSTALLS += settingsqml
