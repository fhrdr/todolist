QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += \
    src \
    src/core \
    src/ui \
    src/ui/components \
    src/ui/widgets

SOURCES += \
    src/main.cpp \
    src/core/todoitem.cpp \
    src/core/todofolder.cpp \
    src/core/databasemanager.cpp \
    src/ui/mainwindow.cpp \
    src/ui/components/navbar.cpp \
    src/ui/components/sectionheader.cpp \
    src/ui/components/titlebar.cpp \
    src/ui/components/flowlayout.cpp \
    src/ui/widgets/desktopwidget.cpp \
    src/ui/widgets/calendarwidget.cpp \
    src/ui/widgets/tagwidget.cpp \
    src/ui/widgets/statswidget.cpp

HEADERS += \
    src/core/todoitem.h \
    src/core/todofolder.h \
    src/core/databasemanager.h \
    src/ui/mainwindow.h \
    src/ui/theme.h \
    src/ui/icons.h \
    src/ui/components/navbar.h \
    src/ui/components/sectionheader.h \
    src/ui/components/titlebar.h \
    src/ui/components/flowlayout.h \
    src/ui/components/messageutils.h \
    src/ui/widgets/desktopwidget.h \
    src/ui/widgets/calendarwidget.h \
    src/ui/widgets/tagwidget.h \
    src/ui/widgets/statswidget.h

RESOURCES += \
    resources.qrc

RC_FILE = todolist_resource.rc

win32: LIBS += -ldwmapi

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
