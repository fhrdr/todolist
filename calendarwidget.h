#ifndef CALENDARWIDGET_H
#define CALENDARWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QScrollArea>
#include <QDate>
#include <QMap>
#include <QList>
#include <QPainter>
#include <QMouseEvent>
#include "todoitem.h"
#include "todofolder.h"

class CalendarCell : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarCell(QWidget *parent = nullptr);
    void setDate(const QDate &date);
    void setTodos(const QList<TodoItem> &todos);
    void setOtherMonth(bool other);
    void setSelected(bool selected);
    void setToday(bool today);
    QDate getDate() const { return m_date; }
    
signals:
    void clicked(const QDate &date);
    void todoClicked(const QString &todoId);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
private:
    QDate m_date;
    QList<TodoItem> m_todos;
    bool m_otherMonth;
    bool m_selected;
    bool m_today;
    bool m_pressed;
    int m_clickedTodoIndex;
    
    QRect getDateRect() const;
    QRect getTodoRect(int index) const;
};

class CalendarGrid : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarGrid(QWidget *parent = nullptr);
    void setCurrentMonth(int year, int month);
    void setTodoData(const QMap<QDate, QList<TodoItem>> &todos);
    void setSelectedDate(const QDate &date);
    QDate getSelectedDate() const { return m_selectedDate; }
    int getYear() const { return m_year; }
    int getMonth() const { return m_month; }
    
signals:
    void dateClicked(const QDate &date);
    void todoClicked(const QString &todoId);
    void monthChanged(int year, int month);
    
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    
private slots:
    void onPrevMonth();
    void onNextMonth();
    void onPrevYear();
    void onNextYear();
    
private:
    void setupUI();
    void updateCells();
    
    QVBoxLayout *m_mainLayout;
    QWidget *m_headerWidget;
    QHBoxLayout *m_headerLayout;
    QPushButton *m_prevYearBtn;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_nextYearBtn;
    QLabel *m_monthLabel;
    QWidget *m_weekHeader;
    QHBoxLayout *m_weekHeaderLayout;
    QWidget *m_gridWidget;
    QGridLayout *m_gridLayout;
    
    QList<CalendarCell*> m_cells;
    int m_year;
    int m_month;
    QDate m_selectedDate;
    QMap<QDate, QList<TodoItem>> m_todoData;
};

class TodoListItem : public QWidget
{
    Q_OBJECT

public:
    explicit TodoListItem(const TodoItem &item, QWidget *parent = nullptr);
    QString getTodoId() const { return m_todoId; }
    void setSelected(bool selected);
    
signals:
    void clicked(const QString &todoId);
    void doubleClicked(const QString &todoId);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    
private:
    QString m_todoId;
    QString m_title;
    bool m_completed;
    QString m_tagColor;
    bool m_selected;
};

class CalendarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarWidget(QWidget *parent = nullptr);
    ~CalendarWidget();
    
    void updateTodoData(const QList<TodoFolder> &folders);
    
signals:
    void todoItemAdded(const QString &title, const QDate &date);
    void todoItemToggled(const QString &itemId, bool completed);
    void todoItemDeleted(const QString &itemId);
    
private slots:
    void onDateClicked(const QDate &date);
    void onTodoClicked(const QString &todoId);
    void onTodoDoubleClicked(const QString &todoId);
    void onAddTodo();
    void onDeleteTodo();
    void onToggleTodo();
    void onPrevMonth();
    void onNextMonth();
    
private:
    void setupUI();
    void setupConnections();
    void refreshTodoList();
    void refreshCalendarData();
    void updateDateLabel();
    
    QHBoxLayout *m_mainLayout;
    
    QWidget *m_leftPanel;
    QVBoxLayout *m_leftLayout;
    CalendarGrid *m_calendarGrid;
    
    QWidget *m_rightPanel;
    QVBoxLayout *m_rightLayout;
    QLabel *m_dateLabel;
    QLabel *m_countLabel;
    QScrollArea *m_todoScrollArea;
    QWidget *m_todoContainer;
    QVBoxLayout *m_todoListLayout;
    QLineEdit *m_addLineEdit;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QPushButton *m_toggleButton;
    QWidget *m_addPanel;
    QHBoxLayout *m_addLayout;
    
    QList<TodoFolder> m_folders;
    QDate m_currentDate;
    QString m_selectedTodoId;
    QMap<QDate, QList<TodoItem>> m_dateToTodos;
    QList<TodoListItem*> m_todoItems;
};

#endif // CALENDARWIDGET_H
