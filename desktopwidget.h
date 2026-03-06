#ifndef DESKTOPWIDGET_H
#define DESKTOPWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>
#include <QPoint>
#include "todoitem.h"
#include "todofolder.h"

class DesktopWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopWidget(QWidget *parent = nullptr);
    ~DesktopWidget();
    
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
    void onRefreshTimer();
    
signals:
    void todoItemToggled(const QString &itemId, bool completed);
    void newTodoRequested(const QString &title);
    void showMainWindowRequested();
    void editTodoRequested(const QString &itemId);
    void deleteTodoRequested(const QString &itemId);
    
private:
    void setupUI();
    void setupConnections();
    void applyStyles();
    void updateTodoList();
    void loadPendingItems();
    void saveWindowPosition();
    void loadWindowPosition();
    
    QVBoxLayout *m_mainLayout;
    QWidget *m_headerWidget;
    QHBoxLayout *m_headerLayout;
    QLabel *m_titleLabel;
    QLabel *m_countLabel;
    
    QListWidget *m_todoListWidget;
    
    QWidget *m_addWidget;
    QHBoxLayout *m_addLayout;
    QLineEdit *m_addLineEdit;
    QPushButton *m_addButton;
    
    QList<TodoFolder> m_folders;
    QList<TodoItem> m_displayItems;
    
    QPoint m_dragPosition;
    bool m_dragging;
    
    QTimer *m_refreshTimer;
};

#endif // DESKTOPWIDGET_H
