QT += core gui network widgets

TARGET = temperature_gui
TEMPLATE = app
CONFIG += c++17

# Подключаем QWT
unix:!macx: LIBS += -lqwt-qt5
INCLUDEPATH += /usr/include/qwt

SOURCES += main.cpp \
           mainwindow.cpp

HEADERS += mainwindow.h
