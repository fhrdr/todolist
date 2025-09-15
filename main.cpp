#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    a.setApplicationName("Todo List");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("TodoApp");
    
    // 设置应用程序图标
    a.setWindowIcon(QIcon("icons/app.ico"));
    
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
