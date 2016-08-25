TEMPLATE = app
QT += core widgets gui opengl
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

CONFIG(debug, release|debug):DEFINES += _DEBUG

OBJECTS_DIR = $$CONFIGDIR/.obj
MOC_DIR = $$CONFIGDIR/.moc
RCC_DIR = $$CONFIGDIR/.rcc
UI_DIR = $$CONFIGDIR/.ui

DESTDIR = $$CONFIGDIR/build

msvc {
    CONFIG(release, debug|release):LIBS += -lassimp
    CONFIG(debug, debug|release):LIBS += -lassimpd
}

gcc {
    LIBS += -lassimp.dll
}

include(MeshReduction.pri)
