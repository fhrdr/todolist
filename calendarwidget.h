#ifndef CALENDARWIDGET_H
#define CALENDARWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDate>
#include <QMap>
#include <QScrollArea>
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
    
signals:
    void dateClicked(const QDate &date);
    void todoClicked(const QString &todoId);
    
private:
    void setupUI();
    void updateCells();
    
    QVBoxLayout *m_mainLayout;
    QWidget *m_headerWidget;
    QHBoxLayout *m_headerLayout;
    QLabel *m_prevBtn;
    QLabel *m_nextBtn;
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
    
signals:
    void clicked(const QString &todoId);
    void toggled(const QString &todoId, bool completed);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    
private:
    QString m_todoId;
    QString m_title;
    bool m_completed;
    QString m_tagColor;
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
    void onAddTodo();
    void onDeleteTodo();
    
private:
    void setupUI();
    void setupConnections();
    void refreshTodoList();
    void refreshCalendarData();
    
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
    
    QWidget *m_addPanel;
    QHBoxLayout *m_addLayout;
    QLineEdit *m_addLineEdit;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    
    QList<TodoFolder> m_folders;
    QDate m_currentDate;
    QString m_selectedTodoId;
    
    QMap<QDate, QList<TodoItem>> m_dateToTodos;
};

#endif // CALENDARWIDGET_H
