TEMPLATE = app
QT += core widgets gui
DEFINES += QT_DLL QT_WIDGETS_LIB
CONFIG += debug_and_release

TARGET = MeshReduction

INCLUDEPATH += ./include

DEPENDPATH += . \
    ./include \
    ./src \
    ./forms \
    ./res \

LIBS += -L$$PWD/lib

CONFIGDIR = .

CONFIG(release, debug|release):CONFIGDIR = ./release
CONFIG(debug, debug|release):CONFIGDIR = ./debug

OBJECTS_DIR = $$CONFIGDIR/.obj

msvc {
    CONFIG(release, debug|release):LIBS += -lassimp
    CONFIG(debug, debug|release):LIBS += -lassimpd
}

gcc {
    LIBS += -lassimp.dll
}

include(MeshReduction.pri)
