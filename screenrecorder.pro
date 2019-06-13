TEMPLATE = subdirs
SUBDIRS = \
    recorder \
    gui \
    icons \
    settings

gui.depends = recorder

OTHER_FILES += \
    rpm/screenrecorder.spec
