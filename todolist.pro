QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        todoitem.cpp \
        todofolder.cpp \
        desktopwidget.cpp \
        calendarwidget.cpp \
        tagwidget.cpp

HEADERS += \
        mainwindow.h \
        todoitem.h \
        todofolder.h \
        desktopwidget.h \
        calendarwidget.h \
        tagwidget.h \
        fontawesome.h \
        messageutils.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

RC_ICONS = icons/app.ico

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
