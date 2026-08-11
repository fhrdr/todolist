#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QByteArray>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QStackedWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>

#include "../core/todoitem.h"
#include "../core/todofolder.h"
#include "widgets/desktopwidget.h"
#include "widgets/calendarwidget.h"
#include "widgets/tagwidget.h"
#include "widgets/statswidget.h"

class NavBar;
class SectionHeader;
class TitleBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 文件夹
    void onNewFolderClicked();
    void onFolderSelectionChanged();
    void onFolderDoubleClicked(QListWidgetItem *item);
    void onFolderContextMenu(const QPoint &pos);
    void onPinFolderClicked();
    void onDeleteFolderClicked();

    // 待办事项
    void onNewTodoClicked();
    void onQuickAddTodo();
    void onTodoSelectionChanged();
    void onTodoClicked(QListWidgetItem *item);
    void onTodoDoubleClicked(QListWidgetItem *item);
    void onTodoContextMenu(const QPoint &pos);
    void onSaveClicked();
    void onDeleteClicked();
    void onCompletedToggled(bool completed);
    void onRemindToggled(bool remind);

    // 标签
    void onTodoTagAdded(const QString &todoId, const QString &tag);
    void onTodoTagRemoved(const QString &todoId, const QString &tag);

    // 文件 / 数据
    void onImportClicked();
    void onExportClicked();
    void onBackupClicked();
    void onOpenBackupDir();
    void onExitClicked();
    void onDesktopWidgetClicked();

    // 搜索
    void onSearchTextChanged(const QString &text);

    // 回收站
    void onTrashClicked();

    // 提醒
    void checkReminders();

    // 主题
    void onToggleDarkMode(bool dark);

    // 桌面小贴士桥接
    void onDesktopNewTodo(const QString &title);
    void onDesktopTodoToggled(const QString &itemId, bool completed);
    void onShowMainWindow();

    // 日历桥接
    void onCalendarTodoAdded(const QString &title, const QDate &date);
    void onCalendarTodoToggled(const QString &itemId, bool completed);
    void onCalendarTodoDeleted(const QString &itemId);

    // 托盘
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowFromTray();
    void onExitFromTray();
    void onAboutToQuit();

private:
    // ---- UI 构建 ----
    void buildUi();
    QWidget* buildListPage();
    QWidget* buildFolderPanel();
    QWidget* buildTodoPanel();
    QWidget* buildDetailPanel();
    void setupConnections();
    void setupSystemTray();
    void setupDesktopWidget();
    void setupCalendarWidget();
    void setupTagWidget();
    void setupSplitter();
    void saveSplitterState();
    void loadSplitterState();

    // ---- 视图刷新 ----
    void updateFolderList();
    void updateTodoList();
    void showSearchResults(const QString &text);
    void updateDetailPanel();
    void clearDetailPanel();
    void updateTodoTags();
    void updateStatusBar();
    void refreshAllViews();
    void updateDesktopWidget();
    void updateCalendarWidget();
    void updateTagWidget();
    void updateStatsWidget();

    // ---- 数据访问（ID 驱动，杜绝悬空指针） ----
    TodoFolder* findFolderById(const QString &folderId);
    TodoItem* findTodoItemById(const QString &itemId, QString *outFolderId = nullptr);
    TodoFolder* currentFolder();
    TodoItem* currentItem();
    bool toggleTodoCompleted(const QString &itemId, bool completed);
    bool deleteTodoItem(const QString &itemId);
    void persistFolder(TodoFolder *folder);
    void persistItem(TodoItem *item);

    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

    // ---- 数据 ----
    QList<TodoFolder> m_folders;
    QString m_currentFolderId;
    QString m_currentItemId;

    // ---- 框架 ----
    TitleBar *m_titleBar = nullptr;
    NavBar *m_navBar = nullptr;
    QStackedWidget *m_stack = nullptr;
    QSplitter *m_mainSplitter = nullptr;

    // 文件夹面板
    QListWidget *m_folderList = nullptr;
    QPushButton *m_newFolderBtn = nullptr;
    QLabel *m_folderEmptyHint = nullptr;

    // 待办面板
    SectionHeader *m_todoHeader = nullptr;
    QListWidget *m_todoList = nullptr;
    QLineEdit *m_quickAddEdit = nullptr;
    QPushButton *m_newTodoBtn = nullptr;
    QLabel *m_todoEmptyHint = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    bool m_searching = false;

    // 详情面板
    QWidget *m_detailCard = nullptr;
    QScrollArea *m_detailScroll = nullptr;
    QLabel *m_emptyStateLabel = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QTextEdit *m_detailsEdit = nullptr;
    QComboBox *m_priorityCombo = nullptr;
    QComboBox *m_tagColorCombo = nullptr;
    QLabel *m_tagsDisplayLabel = nullptr;
    QPushButton *m_addTagBtn = nullptr;
    QLabel *m_createdTimeLabel = nullptr;
    QLabel *m_completedTimeTitle = nullptr;
    QLabel *m_completedTimeLabel = nullptr;
    QCheckBox *m_completedCheck = nullptr;
    QCheckBox *m_remindCheck = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;

    // 子视图
    DesktopWidget *m_desktopWidget = nullptr;
    CalendarWidget *m_calendarWidget = nullptr;
    TagWidget *m_tagWidget = nullptr;
    StatsWidget *m_statsWidget = nullptr;
    QTimer *m_reminderTimer = nullptr;

    // 托盘
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;

    QListWidgetItem *m_dragHoverItem = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // MAINWINDOW_H
