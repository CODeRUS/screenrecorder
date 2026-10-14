TEMPLATE = subdirs
SUBDIRS = \
    recorder \
    gui \
    settings

gui.depends = recorder

OTHER_FILES += \
    rpm/screenrecorder.spec
