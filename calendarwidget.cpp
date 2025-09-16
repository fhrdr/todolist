#include "calendarwidget.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCharFormat>
#include <QHeaderView>
#include <QSizePolicy>

CalendarWidget::CalendarWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentDate(QDate::currentDate())
    , m_currentItem(nullptr)
{
    // 初始化m_folders为空列表
    m_folders.clear();
    
    setupUI();
    setupConnections();
    applyStyles();
    
    // 设置当前日期并高亮今天
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
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // 顶部布局
    m_topLayout = new QHBoxLayout();
    
    // 日历组件
    m_calendar = new QCalendarWidget();
    m_calendar->setMinimumSize(350, 250);
    m_calendar->setMaximumSize(400, 300);
    
    // 隐藏周数列（最左侧一列）
    m_calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    
    // 设置导航栏格式，去掉月份下面的下标
    m_calendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    m_calendar->setNavigationBarVisible(true);
    
    // 设置今天为默认选中日期并高亮显示
    m_calendar->setSelectedDate(QDate::currentDate());
    
    // 美化日历样式
    m_calendar->setGridVisible(true);
    
    // 待办事项区域
    m_todoWidget = new QWidget();
    m_todoLayout = new QVBoxLayout(m_todoWidget);
    m_todoLayout->setContentsMargins(5, 5, 5, 5);
    
    // 日期标签
    m_dateLabel = new QLabel();
    m_dateLabel->setAlignment(Qt::AlignCenter);
    m_dateLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333; padding: 5px;");
    m_todoLayout->addWidget(m_dateLabel);
    
    // 待办事项数量标签
    m_todoCountLabel = new QLabel();
    m_todoCountLabel->setAlignment(Qt::AlignCenter);
    m_todoCountLabel->setStyleSheet("font-size: 12px; color: #666; padding: 2px;");
    m_todoLayout->addWidget(m_todoCountLabel);
    
    // 待办事项列表
    m_todoListWidget = new QListWidget();
    m_todoListWidget->setMinimumHeight(200);
    m_todoLayout->addWidget(m_todoListWidget);
    
    // 添加待办事项布局
    m_addLayout = new QHBoxLayout();
    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("为此日期添加待办事项...");
    m_addButton = new QPushButton("添加");
    m_addButton->setMaximumWidth(60);
    
    m_addLayout->addWidget(m_addLineEdit);
    m_addLayout->addWidget(m_addButton);
    m_todoLayout->addLayout(m_addLayout);
    
    // 操作按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(8);
    
    m_deleteButton = new QPushButton("🗑️ 删除");
    m_refreshButton = new QPushButton("🔄 刷新");
    
    // 设置按钮样式
    QString buttonStyle = 
        "QPushButton {"
        "    padding: 6px 12px;"
        "    border: 1px solid #ddd;"
        "    border-radius: 6px;"
        "    background-color: #f8f9fa;"
        "    font-size: 11px;"
        "    font-weight: 500;"
        "    min-width: 60px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e9ecef;"
        "    border-color: #adb5bd;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #dee2e6;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #f8f9fa;"
        "    color: #6c757d;"
        "    border-color: #dee2e6;"
        "}";
    
    m_deleteButton->setStyleSheet(buttonStyle + 
        "QPushButton:hover { background-color: #f8d7da; border-color: #f5c6cb; }");
    m_refreshButton->setStyleSheet(buttonStyle + 
        "QPushButton:hover { background-color: #cce5ff; border-color: #99d6ff; }");
    
    m_deleteButton->setEnabled(false);
    
    m_buttonLayout->addWidget(m_deleteButton);
    m_buttonLayout->addWidget(m_refreshButton);
    m_todoLayout->addLayout(m_buttonLayout);
    
    // 添加到主布局
    m_topLayout->addWidget(m_calendar);
    m_topLayout->addWidget(m_todoWidget, 1);
    
    m_mainLayout->addLayout(m_topLayout);
}

void CalendarWidget::setupConnections()
{
    connect(m_calendar, &QCalendarWidget::clicked, this, &CalendarWidget::onDateChanged);
    connect(m_addButton, &QPushButton::clicked, this, &CalendarWidget::onAddTodoForDate);
    connect(m_addLineEdit, &QLineEdit::returnPressed, this, &CalendarWidget::onAddTodoForDate);
    connect(m_todoListWidget, &QListWidget::itemClicked, this, &CalendarWidget::onTodoItemClicked);
    connect(m_todoListWidget, &QListWidget::itemChanged, this, &CalendarWidget::onTodoItemChanged);
    connect(m_deleteButton, &QPushButton::clicked, this, &CalendarWidget::onDeleteSelectedTodo);
    connect(m_refreshButton, &QPushButton::clicked, this, &CalendarWidget::onRefreshClicked);
}

void CalendarWidget::applyStyles()
{
    setStyleSheet(
        "CalendarWidget {"
        "    background-color: #f8f9fa;"
        "}"
        "QCalendarWidget {"
        "    background-color: white;"
        "    border: 1px solid #ddd;"
        "    border-radius: 8px;"
        "    font-family: 'Microsoft YaHei', Arial, sans-serif;"
        "}"
        "QCalendarWidget QWidget#qt_calendar_navigationbar {"
        "    background-color: #4a90e2;"
        "    border-top-left-radius: 8px;"
        "    border-top-right-radius: 8px;"
        "}"
        "QCalendarWidget QTableView {"
        "    outline: 0px;"
        "    gridline-color: #e0e0e0;"
        "    background-color: white;"
        "    selection-background-color: #4a90e2;"
        "    selection-color: white;"
        "}"
        "QCalendarWidget QTableView::item {"
        "    padding: 8px;"
        "    border: none;"
        "}"
        "QCalendarWidget QTableView::item:hover {"
        "    background-color: #e3f2fd;"
        "}"
        "QCalendarWidget QTableView::item:selected {"
        "    background-color: #4a90e2;"
        "    color: white;"
        "    font-weight: bold;"
        "}"
        "QCalendarWidget QToolButton {"
        "    height: 30px;"
        "    width: 60px;"
        "    color: #333;"
        "    font-size: 12px;"
        "    border: none;"
        "}"
        "QCalendarWidget QMenu {"
        "    width: 120px;"
        "    color: #333;"
        "    background-color: white;"
        "    border: 1px solid #ddd;"
        "}"
        "QCalendarWidget QSpinBox {"
        "    width: 60px;"
        "    font-size: 12px;"
        "    color: #333;"
        "    background-color: white;"
        "    border: 1px solid #ddd;"
        "    border-radius: 3px;"
        "}"
        "QCalendarWidget QAbstractItemView:enabled {"
        "    font-size: 11px;"
        "    color: #333;"
        "    background-color: white;"
        "    selection-background-color: #64b5f6;"
        "}"
        "QListWidget {"
        "    background-color: white;"
        "    border: 1px solid #ddd;"
        "    border-radius: 5px;"
        "    font-size: 12px;"
        "}"
        "QListWidget::item {"
        "    padding: 8px;"
        "    border-bottom: 1px solid #eee;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #f0f8ff;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #e3f2fd;"
        "    color: #1976d2;"
        "}"
        "QLineEdit {"
        "    padding: 8px;"
        "    border: 1px solid #ddd;"
        "    border-radius: 4px;"
        "    font-size: 12px;"
        "}"
        "QPushButton {"
        "    padding: 8px 16px;"
        "    border: 1px solid #ddd;"
        "    border-radius: 4px;"
        "    background-color: #f8f9fa;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e9ecef;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #dee2e6;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #f8f9fa;"
        "    color: #6c757d;"
        "    border-color: #dee2e6;"
        "}"
        "QTextEdit {"
        "    border: 1px solid #ddd;"
        "    border-radius: 4px;"
        "    font-size: 12px;"
        "    background-color: white;"
        "}"
    );
}

void CalendarWidget::updateTodoData(const QList<TodoFolder> &folders)
{
    // 安全检查：确保传入的数据有效
    if (&folders == &m_folders) {
        // 如果传入的是同一个对象的引用，直接返回避免自赋值
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
    
    // 重新构建日期-待办事项映射
    for (const TodoFolder &folder : m_folders) {
        QList<TodoItem> items = folder.getItems();
        for (const TodoItem &item : items) {
            QDate itemDate = item.getPlannedDate(); // 使用计划日期而不是创建时间
            
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
    updateDateTodoList();
    clearTodoDetails();
}

void CalendarWidget::updateDateTodoList()
{
    m_todoListWidget->clear();
    
    // 更新日期标签
    m_dateLabel->setText(m_currentDate.toString("yyyy年MM月dd日 dddd"));
    
    // 获取当前日期的待办事项
    QList<TodoItem> todos = getTodosForDate(m_currentDate);
    
    // 更新数量标签
    int completedCount = m_dateToCompletedCount.value(m_currentDate, 0);
    int totalCount = m_dateToTotalCount.value(m_currentDate, 0);
    m_todoCountLabel->setText(QString("共 %1 项，已完成 %2 项").arg(totalCount).arg(completedCount));
    
    // 添加待办事项到列表
    for (const TodoItem &item : todos) {
        QListWidgetItem *listItem = new QListWidgetItem();
        listItem->setText(item.getTitle());
        listItem->setData(Qt::UserRole, item.getId());
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
        listItem->setCheckState(item.isCompleted() ? Qt::Checked : Qt::Unchecked);
        
        // 设置字体样式
        QFont font = listItem->font();
        if (item.isCompleted()) {
            font.setStrikeOut(true);
            listItem->setForeground(QColor(128, 128, 128));
        } else {
            font.setStrikeOut(false);
            listItem->setForeground(QColor(0, 0, 0));
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
        clearTodoDetails();
        return;
    }
    
    QString itemId = item->data(Qt::UserRole).toString();
    
    // 查找对应的TodoItem并创建副本
    m_currentItem = nullptr;
    QList<TodoItem> todos = getTodosForDate(m_currentDate);
    for (const TodoItem &todo : todos) {
        if (todo.getId() == itemId) {
            // 创建一个成员变量副本用于编辑
            m_editItem = todo;
            m_currentItem = &m_editItem;
            break;
        }
    }
    
    if (m_currentItem) {
        // 更新按钮状态
        m_deleteButton->setEnabled(true);
    }
}

void CalendarWidget::onTodoItemChanged(QListWidgetItem *item)
{
    if (!item) return;
    
    QString itemId = item->data(Qt::UserRole).toString();
    bool completed = (item->checkState() == Qt::Checked);
    
    emit todoItemToggled(itemId, completed);
}

void CalendarWidget::onDeleteSelectedTodo()
{
    if (!m_currentItem) return;
    
    int ret = QMessageBox::question(this, "确认删除",
                                   QString("确定要删除待办事项 '%1' 吗？").arg(m_currentItem->getTitle()),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        emit todoItemDeleted(m_currentItem->getId());
        clearTodoDetails();
    }
}

// onMarkTodoCompleted 和 onMarkTodoIncomplete 方法已移除

void CalendarWidget::onRefreshClicked()
{
    // 刷新日历数据和待办事项列表
    refreshCalendarData();
    updateDateTodoList();
    highlightDatesWithTodos();
    clearTodoDetails();
}

// updateTodoDetails方法已移除，不再支持详情编辑

void CalendarWidget::clearTodoDetails()
{
    m_currentItem = nullptr;
    
    m_deleteButton->setEnabled(false);
}

void CalendarWidget::updateCalendarHighlights()
{
    highlightDatesWithTodos();
}

void CalendarWidget::highlightDatesWithTodos()
{
    QDate today = QDate::currentDate();
    
    // 为有待办事项的日期设置高亮
    for (auto it = m_dateToTodos.begin(); it != m_dateToTodos.end(); ++it) {
        QDate date = it.key();
        int totalCount = m_dateToTotalCount.value(date, 0);
        int completedCount = m_dateToCompletedCount.value(date, 0);
        
        if (totalCount > 0) {
            QTextCharFormat format;
            
            // 如果是今天，保持今天的特殊样式
            if (date == today) {
                format.setForeground(QBrush(QColor(26, 115, 232)));
                format.setFontWeight(QFont::Bold);
                format.setProperty(QTextFormat::OutlinePen, QPen(QColor(26, 115, 232), 2));
            }
            
            if (completedCount == totalCount) {
                // 全部完成 - 绿色背景
                format.setBackground(QBrush(QColor(200, 255, 200)));
            } else if (completedCount > 0) {
                // 部分完成 - 黄色背景
                format.setBackground(QBrush(QColor(255, 255, 200)));
            } else {
                // 未完成 - 浅红色背景
                format.setBackground(QBrush(QColor(255, 220, 220)));
            }
            
            m_calendar->setDateTextFormat(date, format);
        }
    }
    
    // 确保今天始终有特殊标识
    highlightToday();
}

void CalendarWidget::highlightToday()
{
    QDate today = QDate::currentDate();
    QTextCharFormat todayFormat;
    
    // 设置今天的特殊样式 - 蓝色边框和粗体
    todayFormat.setForeground(QBrush(QColor(26, 115, 232))); // 蓝色文字
    todayFormat.setFontWeight(QFont::Bold);
    todayFormat.setProperty(QTextFormat::OutlinePen, QPen(QColor(26, 115, 232), 2));
    
    // 设置今日的灰色背景
    todayFormat.setBackground(QBrush(QColor(220, 220, 220))); // 灰色背景
    
    m_calendar->setDateTextFormat(today, todayFormat);
}