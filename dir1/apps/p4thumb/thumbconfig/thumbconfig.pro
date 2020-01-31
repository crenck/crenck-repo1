QT += core
QT -= gui

TARGET = thumbconfig
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/json/savethumbconfig
INSTALLS += target

SOURCES += main.cpp \
    dimension.cpp \
    changelistrange.cpp \
    settings.cpp \
    connection.cpp \
    conversion.cpp \
    config.cpp

HEADERS += \
    dimension.h \
    changelistrange.h \
    settings.h \
    connection.h \
    conversion.h \
    config.h
