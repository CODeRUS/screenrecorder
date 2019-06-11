TARGET = screenrecorder-gui

QT += dbus

CONFIG += sailfishapp

SOURCES += \
    src/main.cpp

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/pages/FirstPage.qml \
    qml/pages/SecondPage.qml \
    rpm/qml-test.spec \
    translations/*.ts \
    qml-test.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 256x256

CONFIG += sailfishapp_i18n
TRANSLATIONS += \
    translations/screenrecorder-gui.ts

DISTFILES += \
    qml/screenrecorder-gui.qml

INSTALLS += target
