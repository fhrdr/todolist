#include "calendarwidget.h"
#include <QMessageBox>
#include <QTextCharFormat>
#include <QScrollArea>

CalendarWidget::CalendarWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentDate(QDate::currentDate())
{
    m_folders.clear();
    
    setupUI();
    setupConnections();
    applyStyles();
    
    m_calendar->setSelectedDate(m_currentDate);
    highlightToday();
    onDateChanged(m_currentDate);
}

CalendarWidget::~CalendarWidget()
{
}

void CalendarWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(24, 24, 24, 24);
    m_mainLayout->setSpacing(24);
    
    m_contentLayout = new QHBoxLayout();
    m_contentLayout->setSpacing(24);
    
    m_calendarPanel = new QWidget();
    m_calendarPanelLayout = new QVBoxLayout(m_calendarPanel);
    m_calendarPanelLayout->setContentsMargins(0, 0, 0, 0);
    m_calendarPanelLayout->setSpacing(12);
    
    m_calendarTitle = new QLabel("选择日期");
    m_calendarTitle->setStyleSheet("font-weight: 600; font-size: 16px; color: #1e293b;");
    m_calendarPanelLayout->addWidget(m_calendarTitle);
    
    m_calendar = new QCalendarWidget();
    m_calendar->setMinimumSize(380, 320);
    m_calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    m_calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    m_calendar->setNavigationBarVisible(true);
    m_calendar->setGridVisible(false);
    m_calendar->setSelectedDate(QDate::currentDate());
    m_calendarPanelLayout->addWidget(m_calendar);
    
    m_todoPanel = new QWidget();
    m_todoPanelLayout = new QVBoxLayout(m_todoPanel);
    m_todoPanelLayout->setContentsMargins(0, 0, 0, 0);
    m_todoPanelLayout->setSpacing(16);
    
    m_dateLabel = new QLabel();
    m_dateLabel->setStyleSheet("font-weight: 600; font-size: 16px; color: #1e293b;");
    m_todoPanelLayout->addWidget(m_dateLabel);
    
    m_todoCountLabel = new QLabel();
    m_todoCountLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    m_todoPanelLayout->addWidget(m_todoCountLabel);
    
    m_todoListWidget = new QListWidget();
    m_todoListWidget->setMinimumHeight(280);
    m_todoListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_todoPanelLayout->addWidget(m_todoListWidget);
    
    m_addPanel = new QWidget();
    m_addLayout = new QHBoxLayout(m_addPanel);
    m_addLayout->setContentsMargins(0, 0, 0, 0);
    m_addLayout->setSpacing(8);
    
    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("添加新的待办事项...");
    m_addLayout->addWidget(m_addLineEdit);
    
    m_addButton = new QPushButton("添加");
    m_addButton->setFixedWidth(80);
    m_addLayout->addWidget(m_addButton);
    
    m_todoPanelLayout->addWidget(m_addPanel);
    
    m_buttonPanel = new QWidget();
    m_buttonLayout = new QHBoxLayout(m_buttonPanel);
    m_buttonLayout->setContentsMargins(0, 0, 0, 0);
    m_buttonLayout->setSpacing(12);
    
    m_deleteButton = new QPushButton("删除选中");
    m_deleteButton->setEnabled(false);
    m_buttonLayout->addWidget(m_deleteButton);
    
    m_refreshButton = new QPushButton("刷新");
    m_buttonLayout->addWidget(m_refreshButton);
    
    m_buttonLayout->addStretch();
    m_todoPanelLayout->addWidget(m_buttonPanel);
    
    m_contentLayout->addWidget(m_calendarPanel);
    m_contentLayout->addWidget(m_todoPanel, 1);
    
    m_mainLayout->addLayout(m_contentLayout);
}

void CalendarWidget::setupConnections()
{
    connect(m_calendar, &QCalendarWidget::clicked, this, &CalendarWidget::onDateChanged);
    connect(m_addButton, &QPushButton::clicked, this, &CalendarWidget::onAddTodoForDate);
    connect(m_addLineEdit, &QLineEdit::returnPressed, this, &CalendarWidget::onAddTodoForDate);
    connect(m_todoListWidget, &QListWidget::itemClicked, this, &CalendarWidget::onTodoItemClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &CalendarWidget::onDeleteSelectedTodo);
    connect(m_refreshButton, &QPushButton::clicked, this, &CalendarWidget::onRefreshClicked);
}

void CalendarWidget::applyStyles()
{
    QString panelStyle = 
        "QWidget { background-color: #ffffff; border-radius: 12px; }"
        "QLabel { background-color: transparent; }";
    
    m_calendarPanel->setStyleSheet(panelStyle);
    m_calendarPanel->setStyleSheet("QWidget { background-color: transparent; }");
    
    m_todoPanel->setStyleSheet(
        "QWidget { background-color: #ffffff; border-radius: 12px; }"
        "QLabel { background-color: transparent; }"
        "QListWidget { background-color: transparent; }"
        "QLineEdit { background-color: transparent; }"
        "QPushButton { background-color: transparent; }"
    );
    
    QString todoItemStyle = 
        "QListWidget { border: none; background-color: transparent; }"
        "QListWidget::item { padding: 12px 16px; border-radius: 8px; margin: 2px 0; }"
        "QListWidget::item:hover { background-color: #f1f5f9; }"
        "QListWidget::item:selected { background-color: #eef2ff; color: #4f46e5; }";
    
    m_todoListWidget->setStyleSheet(todoItemStyle);
    
    m_addLineEdit->setStyleSheet(
        "QLineEdit { padding: 12px 16px; border: 1px solid #e2e8f0; border-radius: 8px; background-color: #ffffff; }"
        "QLineEdit:focus { border-color: #6366f1; }"
    );
    
    m_addButton->setStyleSheet(
        "QPushButton { background-color: #6366f1; color: white; border: none; border-radius: 8px; padding: 12px 20px; font-weight: 500; }"
        "QPushButton:hover { background-color: #4f46e5; }"
    );
    
    m_deleteButton->setStyleSheet(
        "QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 8px; padding: 10px 20px; font-weight: 500; }"
        "QPushButton:hover { background-color: #dc2626; }"
        "QPushButton:disabled { background-color: #cbd5e1; color: #94a3b8; }"
    );
    
    m_refreshButton->setStyleSheet(
        "QPushButton { background-color: #8b5cf6; color: white; border: none; border-radius: 8px; padding: 10px 20px; font-weight: 500; }"
        "QPushButton:hover { background-color: #7c3aed; }"
    );
}

void CalendarWidget::updateTodoData(const QList<TodoFolder> &folders)
{
    if (&folders == &m_folders) {
        return;
    }
    
    m_folders = folders;
    refreshCalendarData();
    updateDateTodoList();
    highlightDatesWithTodos();
}

void CalendarWidget::refreshCalendarData()
{
    m_dateToTodos.clear();
    m_dateToCompletedCount.clear();
    m_dateToTotalCount.clear();
    
    for (const TodoFolder &folder : m_folders) {
        QList<TodoItem> items = folder.getItems();
        for (const TodoItem &item : items) {
            QDate itemDate = item.getPlannedDate();
            
            if (!m_dateToTodos.contains(itemDate)) {
                m_dateToTodos[itemDate] = QList<TodoItem>();
                m_dateToCompletedCount[itemDate] = 0;
                m_dateToTotalCount[itemDate] = 0;
            }
            
            m_dateToTodos[itemDate].append(item);
            m_dateToTotalCount[itemDate]++;
            
            if (item.isCompleted()) {
                m_dateToCompletedCount[itemDate]++;
            }
        }
    }
}

QList<TodoItem> CalendarWidget::getTodosForDate(const QDate &date) const
{
    return m_dateToTodos.value(date, QList<TodoItem>());
}

void CalendarWidget::onDateChanged(const QDate &date)
{
    m_currentDate = date;
    m_calendar->setSelectedDate(date);
    m_currentItemId.clear();
    m_deleteButton->setEnabled(false);
    updateDateTodoList();
}

void CalendarWidget::updateDateTodoList()
{
    m_todoListWidget->clear();
    
    m_dateLabel->setText(m_currentDate.toString("yyyy年MM月dd日 dddd"));
    
    QList<TodoItem> todos = getTodosForDate(m_currentDate);
    
    int completedCount = m_dateToCompletedCount.value(m_currentDate, 0);
    int totalCount = m_dateToTotalCount.value(m_currentDate, 0);
    m_todoCountLabel->setText(QString("共 %1 项，已完成 %2 项").arg(totalCount).arg(completedCount));
    
    for (const TodoItem &item : todos) {
        QListWidgetItem *listItem = new QListWidgetItem();
        
        QString prefix = item.isCompleted() ? "✓ " : "○ ";
        listItem->setText(prefix + item.getTitle());
        listItem->setData(Qt::UserRole, item.getId());
        
        QFont font = listItem->font();
        if (item.isCompleted()) {
            font.setStrikeOut(true);
            listItem->setForeground(QColor(100, 116, 139));
        } else {
            listItem->setForeground(QColor(51, 65, 85));
        }
        listItem->setFont(font);
        
        m_todoListWidget->addItem(listItem);
    }
}

void CalendarWidget::onAddTodoForDate()
{
    QString title = m_addLineEdit->text().trimmed();
    if (!title.isEmpty()) {
        emit todoItemAdded(title, m_currentDate);
        m_addLineEdit->clear();
    }
}

void CalendarWidget::onTodoItemClicked(QListWidgetItem *item)
{
    if (!item) {
        m_currentItemId.clear();
        m_deleteButton->setEnabled(false);
        return;
    }
    
    m_currentItemId = item->data(Qt::UserRole).toString();
    
    QList<TodoItem> todos = getTodosForDate(m_currentDate);
    for (const TodoItem &todo : todos) {
        if (todo.getId() == m_currentItemId) {
            bool newCompleted = !todo.isCompleted();
            emit todoItemToggled(m_currentItemId, newCompleted);
            break;
        }
    }
    
    m_deleteButton->setEnabled(!m_currentItemId.isEmpty());
}

void CalendarWidget::onDeleteSelectedTodo()
{
    if (m_currentItemId.isEmpty()) return;
    
    QString title;
    QList<TodoItem> todos = getTodosForDate(m_currentDate);
    for (const TodoItem &todo : todos) {
        if (todo.getId() == m_currentItemId) {
            title = todo.getTitle();
            break;
        }
    }
    
    int ret = QMessageBox::question(this, "确认删除",
                                   QString("确定要删除待办事项 '%1' 吗？").arg(title),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        emit todoItemDeleted(m_currentItemId);
        m_currentItemId.clear();
        m_deleteButton->setEnabled(false);
    }
}

void CalendarWidget::onRefreshClicked()
{
    refreshCalendarData();
    updateDateTodoList();
    highlightDatesWithTodos();
    m_currentItemId.clear();
    m_deleteButton->setEnabled(false);
}

void CalendarWidget::highlightDatesWithTodos()
{
    QDate today = QDate::currentDate();
    
    for (auto it = m_dateToTodos.begin(); it != m_dateToTodos.end(); ++it) {
        QDate date = it.key();
        int totalCount = m_dateToTotalCount.value(date, 0);
        int completedCount = m_dateToCompletedCount.value(date, 0);
        
        if (totalCount > 0) {
            QTextCharFormat format;
            
            if (completedCount == totalCount) {
                format.setBackground(QBrush(QColor(220, 252, 231)));
            } else if (completedCount > 0) {
                format.setBackground(QBrush(QColor(254, 249, 195)));
            } else {
                format.setBackground(QBrush(QColor(254, 226, 226)));
            }
            
            if (date == today) {
                format.setForeground(QBrush(QColor(99, 102, 241)));
                format.setFontWeight(QFont::Bold);
            }
            
            m_calendar->setDateTextFormat(date, format);
        }
    }
    
    highlightToday();
}

void CalendarWidget::highlightToday()
{
    QDate today = QDate::currentDate();
    QTextCharFormat todayFormat;
    todayFormat.setForeground(QBrush(QColor(99, 102, 241)));
    todayFormat.setFontWeight(QFont::Bold);
    todayFormat.setBackground(QBrush(QColor(238, 242, 255)));
    
    m_calendar->setDateTextFormat(today, todayFormat);
}
