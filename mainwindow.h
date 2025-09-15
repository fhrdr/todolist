#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include "todoitem.h"
#include "todofolder.h"
#include "desktopwidget.h"
#include "calendarwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 文件夹相关槽函数
    void onNewFolderClicked();
    void onFolderSelectionChanged();
    void onDeleteFolderClicked();
    
    // 待办事项相关槽函数
    void onNewTodoClicked();
    void onTodoSelectionChanged();
    void onSaveClicked();
    void onDeleteClicked();
    void onCompletedToggled(bool completed);
    void onSyncClicked();
    
    // 菜单相关槽函数
    void onImportClicked();
    void onExportClicked();
    void onExitClicked();
    void onDesktopWidgetClicked();
    
    // 桌面小贴士相关槽函数
    void onDesktopNewTodo(const QString &title);
    void onDesktopTodoToggled(const QString &itemId, bool completed);
    void onShowMainWindow();
    
    // 日历视图相关槽函数
    void onCalendarTodoAdded(const QString &title, const QDate &date);
    void onCalendarTodoToggled(const QString &itemId, bool completed);
    void onCalendarTodoDeleted(const QString &itemId);
    void onCalendarTodoUpdated(const QString &itemId, const TodoItem &item);
    
    // 系统托盘相关槽函数
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowFromTray();
    void onExitFromTray();

private:
    Ui::MainWindow *ui;
    
    // 数据管理
    QList<TodoFolder> m_folders;
    TodoFolder* m_currentFolder;
    TodoItem* m_currentItem;
    
    // 桌面小贴士
    DesktopWidget* m_desktopWidget;
    
    // 日历视图
    CalendarWidget* m_calendarWidget;
    
    // 系统托盘
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    QAction* m_showAction;
    QAction* m_exitAction;
    
    // 界面更新方法
    void updateFolderList();
    void updateTodoList();
    void updateDetailPanel();
    void clearDetailPanel();
    void updateDesktopWidget();
    void updateCalendarWidget();
    
    // 数据操作方法
    void loadData();
    void saveData();
    TodoFolder* findFolderById(const QString &folderId);
    
    // 初始化方法
    void setupConnections();
    void initializeData();
    void setupDesktopWidget();
    void setupCalendarWidget();
    void setupSystemTray();
    
    // 重写事件处理
    void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
