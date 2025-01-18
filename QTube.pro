QT += core gui
QT += network
QT += widgets quick quickwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/mainwindow.h

FORMS += \
    src/mainwindow.ui

TRANSLATIONS += \
    i18n/QTube_uk_UA.ts

CONFIG += lrelease
CONFIG += embed_translations


target.path = bin


!isEmpty(target.path): INSTALLS += target
