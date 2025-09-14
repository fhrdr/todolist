#ifndef CALENDARWIDGET_H
#define CALENDARWIDGET_H

#include <QWidget>
#include <QCalendarWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QDate>
#include <QMap>
#include <QSplitter>
#include "todoitem.h"
#include "todofolder.h"

class CalendarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarWidget(QWidget *parent = nullptr);
    ~CalendarWidget();
    
    // 数据更新接口
    void updateTodoData(const QList<TodoFolder> &folders);
    void refreshCalendarData();
    
    // 获取指定日期的待办事项
    QList<TodoItem> getTodosForDate(const QDate &date) const;
    
public slots:
    void onDateChanged(const QDate &date);
    void onAddTodoForDate();
    void onTodoItemClicked(QListWidgetItem *item);
    void onTodoItemChanged(QListWidgetItem *item);
    void onDeleteSelectedTodo();
    void onMarkTodoCompleted();
    void onMarkTodoIncomplete();
    
signals:
    void todoItemAdded(const QString &title, const QDate &date);
    void todoItemToggled(const QString &itemId, bool completed);
    void todoItemDeleted(const QString &itemId);
    void todoItemUpdated(const QString &itemId, const TodoItem &item);
    
private slots:
    void updateDateTodoList();
    void updateTodoDetails();
    void clearTodoDetails();
    
private:
    void setupUI();
    void setupConnections();
    void applyStyles();
    void updateCalendarHighlights();
    void highlightDatesWithTodos();
    
    // UI组件
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_topLayout;
    QSplitter *m_splitter;
    
    // 日历相关
    QCalendarWidget *m_calendar;
    QLabel *m_dateLabel;
    
    // 待办事项列表
    QWidget *m_todoWidget;
    QVBoxLayout *m_todoLayout;
    QLabel *m_todoCountLabel;
    QListWidget *m_todoListWidget;
    
    // 添加待办事项
    QHBoxLayout *m_addLayout;
    QLineEdit *m_addLineEdit;
    QPushButton *m_addButton;
    
    // 操作按钮
    QHBoxLayout *m_buttonLayout;
    QPushButton *m_deleteButton;
    QPushButton *m_completeButton;
    QPushButton *m_incompleteButton;
    
    // 详情面板
    QWidget *m_detailWidget;
    QVBoxLayout *m_detailLayout;
    QLabel *m_detailTitleLabel;
    QTextEdit *m_detailTextEdit;
    QPushButton *m_saveDetailButton;
    
    // 数据管理
    QList<TodoFolder> m_folders;
    QDate m_currentDate;
    TodoItem *m_currentItem;
    
    // 日期-待办事项映射（用于快速查找和高亮显示）
    QMap<QDate, QList<TodoItem>> m_dateToTodos;
    QMap<QDate, int> m_dateToCompletedCount;
    QMap<QDate, int> m_dateToTotalCount;
};

#endif // CALENDARWIDGET_H