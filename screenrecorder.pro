TEMPLATE = subdirs
SUBDIRS = \
    recorder \
    gui

gui.depends = recorder

OTHER_FILES += \
    rpm/screenrecorder.spec
