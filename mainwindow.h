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
#include <QSqlDatabase>
#include <QSplitter>
#include <QSettings>
#include "todoitem.h"
#include "todofolder.h"
#include "desktopwidget.h"
#include "calendarwidget.h"
#include "tagwidget.h"

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
    void onNewFolderClicked();
    void onFolderSelectionChanged();
    void onDeleteFolderClicked();
    void onPinFolderClicked();
    void onFolderDoubleClicked(QListWidgetItem* item);
    void onFolderContextMenu(const QPoint &pos);
    
    void onNewTodoClicked();
    void onTodoSelectionChanged();
    void onTodoDoubleClicked(QListWidgetItem* item);
    void onTodoContextMenu(const QPoint &pos);
    void onSaveClicked();
    void onDeleteClicked();
    void onCompletedToggled(bool completed);
    void onSyncClicked();
    void onTagSelected(const QString &tag);
    void onTodoTagAdded(const QString &todoId, const QString &tag);
    void onTodoTagRemoved(const QString &todoId, const QString &tag);
    
    void onImportClicked();
    void onExportClicked();
    void onExitClicked();
    void onDesktopWidgetClicked();
    
    void onDesktopNewTodo(const QString &title);
    void onDesktopTodoToggled(const QString &itemId, bool completed);
    void onShowMainWindow();
    
    void onCalendarTodoAdded(const QString &title, const QDate &date);
    void onCalendarTodoToggled(const QString &itemId, bool completed);
    void onCalendarTodoDeleted(const QString &itemId);
    
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowFromTray();
    void onExitFromTray();

private:
    Ui::MainWindow *ui;
    
    QList<TodoFolder> m_folders;
    TodoFolder* m_currentFolder;
    TodoItem* m_currentItem;
    QString m_selectedTag;
    
    DesktopWidget* m_desktopWidget;
    CalendarWidget* m_calendarWidget;
    TagWidget* m_tagWidget;
    
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    QAction* m_showAction;
    QAction* m_exitAction;
    
    QSqlDatabase m_db;
    QSplitter *m_mainSplitter;
    
    void updateFolderList();
    void updateTodoList();
    void updateDetailPanel();
    void clearDetailPanel();
    void updateDesktopWidget();
    void updateCalendarWidget();
    void updateTagWidget();
    void updateTodoTags();
    
    void initDatabase();
    void loadData();
    void saveData();
    void migrateFromJson();
    TodoFolder* findFolderById(const QString &folderId);
    TodoFolder* findOrCreateTodayFolder();
    
    void setupConnections();
    void initializeData();
    void setupDesktopWidget();
    void setupCalendarWidget();
    void setupTagWidget();
    void setupSystemTray();
    void setupSplitter();
    void saveSplitterState();
    void loadSplitterState();
    
    void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
