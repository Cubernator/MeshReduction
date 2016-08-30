HEADERS += \
    $$PWD/include/mesh.hpp \
    $$PWD/include/meshreduction.hpp \
    $$PWD/include/scenefile.hpp \
    $$PWD/include/util.hpp \
    $$PWD/include/mesh_iterators.hpp \
    $$PWD/include/mesh_index.hpp \
    $$PWD/include/mesh_decimator.hpp \
    $$PWD/include/exportdialog.hpp \
    $$PWD/include/mesh_viewer.hpp

SOURCES += \
    $$PWD/src/main.cpp \
    $$PWD/src/meshreduction.cpp \
    $$PWD/src/scenefile.cpp \
    $$PWD/src/mesh.cpp \
    $$PWD/src/mesh_iterators.cpp \
    $$PWD/src/util.cpp \
    $$PWD/src/mesh_decimator.cpp \
    $$PWD/src/exportdialog.cpp \
    $$PWD/src/mesh_viewer.cpp

FORMS += \
    $$PWD/forms/meshreduction.ui \
    $$PWD/forms/exportdialog.ui

RESOURCES += \
    $$PWD/res/shader.qrc

DISTFILES +=
