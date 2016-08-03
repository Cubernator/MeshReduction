HEADERS += \
    $$PWD/include/glwidget.hpp \
    $$PWD/include/mesh.hpp \
    $$PWD/include/meshreduction.hpp \
    $$PWD/include/scenefile.hpp \
    $$PWD/include/util.hpp \
    $$PWD/include/mesh_iterators.hpp \
    $$PWD/include/mesh_index.hpp

SOURCES += \
    $$PWD/src/glwidget.cpp \
    $$PWD/src/main.cpp \
    $$PWD/src/meshreduction.cpp \
    $$PWD/src/scenefile.cpp \
    $$PWD/src/mesh.cpp \
    $$PWD/src/mesh_iterators.cpp

FORMS += $$PWD/forms/meshreduction.ui

RESOURCES += \
    $$PWD/res/shader.qrc

DISTFILES +=
