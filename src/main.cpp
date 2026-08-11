#include "mainwindow.h"
#include "fontawesome.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QIcon>
#include <QSharedMemory>
#include <QMessageBox>
#include <QFontDatabase>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    QSharedMemory sharedMemory("TodoListApp_SingleInstance");
    if (!sharedMemory.create(1)) {
        QMessageBox::information(nullptr, "Todo List", "程序已经在运行中！");
        return 0;
    }
    
    a.setApplicationName("Todo List");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("TodoApp");
    
    QIcon appIcon(":/icons/app.ico");
    if (appIcon.isNull()) {
        QString iconPath = QDir::currentPath() + "/icons/app.ico";
        appIcon = QIcon(iconPath);
        if (appIcon.isNull()) {
            appIcon = QIcon("icons/app.ico");
        }
    }
    a.setWindowIcon(appIcon);
    
    QString fontPath = QDir::currentPath() + "/fonts/Font Awesome 7 Free-Solid-900.otf";
    if (QFile::exists(fontPath)) {
        int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId >= 0) {
            QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            qDebug() << "Font Awesome loaded:" << families;
        } else {
            qDebug() << "Failed to load Font Awesome from:" << fontPath;
        }
    } else {
        qDebug() << "Font Awesome file not found:" << fontPath;
    }
    
    QFile styleFile(":/styles.qss");
    if (!styleFile.exists()) {
        styleFile.setFileName("styles.qss");
    }
    
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        QString styleSheet = stream.readAll();
        a.setStyleSheet(styleSheet);
        styleFile.close();
    }
    
    MainWindow w;
    w.show();

    return a.exec();
}
