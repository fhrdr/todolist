#ifndef DESKTOPWIDGET_H
#define DESKTOPWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include "todoitem.h"
#include "todofolder.h"

class DesktopWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopWidget(QWidget *parent = nullptr);
    ~DesktopWidget();
    
    // 数据更新接口
    void updateTodoData(const QList<TodoFolder> &folders);
    void refreshDisplay();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    void onAddTodoClicked();
    void onTodoItemClicked(QListWidgetItem *item);
    void onTodoItemChanged(QListWidgetItem *item);
    void onRefreshTimer();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowMainWindow();
    void onExitApplication();
    
signals:
    void todoItemToggled(const QString &itemId, bool completed);
    void newTodoRequested(const QString &title);
    void showMainWindowRequested();
    
private:
    // UI组件
    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QListWidget *m_todoListWidget;
    QHBoxLayout *m_addLayout;
    QLineEdit *m_addLineEdit;
    QPushButton *m_addButton;
    QPushButton *m_refreshButton;
    QPushButton *m_settingsButton;
    
    // 系统托盘
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_showAction;
    QAction *m_exitAction;
    
    // 数据
    QList<TodoFolder> m_folders;
    QList<TodoItem> m_displayItems; // 当前显示的待办事项
    
    // 窗口拖拽
    QPoint m_dragPosition;
    bool m_dragging;
    
    // 定时器
    QTimer *m_refreshTimer;
    
    // 私有方法
    void setupUI();
    void setupTrayIcon();
    void setupConnections();
    void updateTodoList();
    void loadPendingItems();
    void applyStyles();
    void saveWindowPosition();
    void loadWindowPosition();
};

#endif // DESKTOPWIDGET_H