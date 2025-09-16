#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QIcon>
#include <QSharedMemory>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 单实例控制
    QSharedMemory sharedMemory("TodoListApp_SingleInstance");
    if (!sharedMemory.create(1)) {
        // 如果共享内存已存在，说明程序已经在运行
        QMessageBox::information(nullptr, "Todo List", "程序已经在运行中！");
        return 0;
    }
    
    // 设置应用程序信息
    a.setApplicationName("Todo List");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("TodoApp");
    
    // 设置应用程序图标（用于任务栏）
    QIcon appIcon(":/icons/app.ico");
    if (appIcon.isNull()) {
        // 如果资源图标加载失败，尝试文件路径
        QString iconPath = QDir::currentPath() + "/icons/app.ico";
        appIcon = QIcon(iconPath);
        if (appIcon.isNull()) {
            appIcon = QIcon("icons/app.ico");
        }
    }
    a.setWindowIcon(appIcon);
    
    // 加载样式表
    QFile styleFile(":/styles.qss");
    if (!styleFile.exists()) {
        // 如果资源文件不存在，尝试从当前目录加载
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
