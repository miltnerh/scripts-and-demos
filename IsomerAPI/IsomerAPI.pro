# IsomerAPI.pro — qmake project file for Qt5/Qt6

QT       += widgets sql
CONFIG   += c++17 console
CONFIG   -= app_bundle

TEMPLATE = app
TARGET   = IsomerAPI

# Sources
SOURCES += \
    main.cpp \
    cpp_isomerapi.cpp \
    levelschemewindow.cpp \
    paintgraphicsview.cpp

# Headers
HEADERS += \
    cpp_isomerapi.h \
    levelschemewindow.h \
    paintgraphicsview.h \
    vectorstructures.h

# UI forms
FORMS += \
    cpp_isomerapi.ui \
    levelschemewindow.ui

# Include path
INCLUDEPATH += $$PWD

# Database file (tracked for IDE, packaging, etc.)
# DISTFILES += database_store/Isomer_DB_WIDGET.sqlite

# Ensure database_store/ exists in build dir, then copy DB after build
# QMAKE_POST_LINK += $$QMAKE_MKDIR \"$$OUT_PWD/database_store\"
# QMAKE_POST_LINK += $$QMAKE_COPY \"$$PWD/database_store/Isomer_DB_WIDGET.sqlite\" \
#                                  \"$$OUT_PWD/database_store/Isomer_DB_WIDGET.sqlite\"

# Handle Qt version differences
greaterThan(QT_MAJOR_VERSION, 5) {
    message("Building with Qt6")
    CONFIG += c++17
} else {
    message("Building with Qt5")
    CONFIG += c++17
}

DISTFILES += \
    database_store/Isomer_DB_WIDGET.sqlite \
    mainstyle.qss
