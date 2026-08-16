#include "ui/mainwindow.h"
#include "ui/theme.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QIcon>
#include <QSettings>
#include <QSharedMemory>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 单实例保护
    QSharedMemory sharedMemory(QStringLiteral("TodoListApp_SingleInstance"));
    if (!sharedMemory.create(1)) {
        QMessageBox::information(nullptr, QStringLiteral("Todo List"),
                                 QStringLiteral("程序已经在运行中！"));
        return 0;
    }

    a.setApplicationName(QStringLiteral("Todo List"));
    a.setApplicationVersion(QStringLiteral("1.0"));
    a.setOrganizationName(QStringLiteral("TodoApp"));

    QIcon appIcon(QStringLiteral(":/icons/app.ico"));
    a.setWindowIcon(appIcon);

    // 主题模式（浅色 / 深色），先于样式表加载
    QSettings settings;
    Theme::setDark(settings.value(QStringLiteral("ui/darkMode"), true).toBool());

    // 全局样式表（资源内嵌，与 Theme 设计令牌一致）
    QFile styleFile(Theme::isDark() ? QStringLiteral(":/styles-dark.qss")
                                    : QStringLiteral(":/styles.qss"));
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        a.setStyleSheet(stream.readAll());
        styleFile.close();
    }

    MainWindow w;
    w.show();

    return a.exec();
}
