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
    setupUI();
    setupConnections();
    applyStyles();
    
    // 设置当前日期
    m_calendar->setSelectedDate(m_currentDate);
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
    
    // 右侧分割器
    m_splitter = new QSplitter(Qt::Horizontal);
    
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
    m_deleteButton = new QPushButton("删除");
    m_completeButton = new QPushButton("完成");
    m_incompleteButton = new QPushButton("未完成");
    
    m_deleteButton->setEnabled(false);
    m_completeButton->setEnabled(false);
    m_incompleteButton->setEnabled(false);
    
    m_buttonLayout->addWidget(m_deleteButton);
    m_buttonLayout->addWidget(m_completeButton);
    m_buttonLayout->addWidget(m_incompleteButton);
    m_todoLayout->addLayout(m_buttonLayout);
    
    // 详情面板
    m_detailWidget = new QWidget();
    m_detailLayout = new QVBoxLayout(m_detailWidget);
    m_detailLayout->setContentsMargins(5, 5, 5, 5);
    
    m_detailTitleLabel = new QLabel("待办事项详情");
    m_detailTitleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333; padding: 5px;");
    m_detailLayout->addWidget(m_detailTitleLabel);
    
    m_detailTextEdit = new QTextEdit();
    m_detailTextEdit->setMaximumHeight(150);
    m_detailTextEdit->setPlaceholderText("选择一个待办事项查看详情...");
    m_detailLayout->addWidget(m_detailTextEdit);
    
    m_saveDetailButton = new QPushButton("保存详情");
    m_saveDetailButton->setEnabled(false);
    m_detailLayout->addWidget(m_saveDetailButton);
    
    // 添加到分割器
    m_splitter->addWidget(m_todoWidget);
    m_splitter->addWidget(m_detailWidget);
    m_splitter->setStretchFactor(0, 2);
    m_splitter->setStretchFactor(1, 1);
    
    // 添加到主布局
    m_topLayout->addWidget(m_calendar);
    m_topLayout->addWidget(m_splitter, 1);
    
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
    connect(m_completeButton, &QPushButton::clicked, this, &CalendarWidget::onMarkTodoCompleted);
    connect(m_incompleteButton, &QPushButton::clicked, this, &CalendarWidget::onMarkTodoIncomplete);
    connect(m_saveDetailButton, &QPushButton::clicked, this, &CalendarWidget::updateTodoDetails);
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
        "    border-radius: 5px;"
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
            QDate itemDate = item.getCreatedTime().date();
            
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
    
    // 查找对应的TodoItem
    m_currentItem = nullptr;
    QList<TodoItem> todos = getTodosForDate(m_currentDate);
    for (const TodoItem &todo : todos) {
        if (todo.getId() == itemId) {
            // 创建一个副本用于编辑
            static TodoItem editItem = todo;
            m_currentItem = &editItem;
            break;
        }
    }
    
    if (m_currentItem) {
        // 更新详情面板
        m_detailTitleLabel->setText(QString("详情: %1").arg(m_currentItem->getTitle()));
        m_detailTextEdit->setPlainText(m_currentItem->getDetails());
        m_saveDetailButton->setEnabled(true);
        
        // 更新按钮状态
        m_deleteButton->setEnabled(true);
        m_completeButton->setEnabled(!m_currentItem->isCompleted());
        m_incompleteButton->setEnabled(m_currentItem->isCompleted());
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

void CalendarWidget::onMarkTodoCompleted()
{
    if (!m_currentItem) return;
    
    emit todoItemToggled(m_currentItem->getId(), true);
}

void CalendarWidget::onMarkTodoIncomplete()
{
    if (!m_currentItem) return;
    
    emit todoItemToggled(m_currentItem->getId(), false);
}

void CalendarWidget::updateTodoDetails()
{
    if (!m_currentItem) return;
    
    QString newDetails = m_detailTextEdit->toPlainText();
    m_currentItem->setDetails(newDetails);
    
    emit todoItemUpdated(m_currentItem->getId(), *m_currentItem);
}

void CalendarWidget::clearTodoDetails()
{
    m_currentItem = nullptr;
    m_detailTitleLabel->setText("待办事项详情");
    m_detailTextEdit->clear();
    m_saveDetailButton->setEnabled(false);
    
    m_deleteButton->setEnabled(false);
    m_completeButton->setEnabled(false);
    m_incompleteButton->setEnabled(false);
}

void CalendarWidget::updateCalendarHighlights()
{
    highlightDatesWithTodos();
}

void CalendarWidget::highlightDatesWithTodos()
{
    // 清除之前的格式
    QTextCharFormat defaultFormat;
    defaultFormat.setBackground(QBrush(Qt::white));
    
    // 为有待办事项的日期设置高亮
    for (auto it = m_dateToTodos.begin(); it != m_dateToTodos.end(); ++it) {
        QDate date = it.key();
        int totalCount = m_dateToTotalCount.value(date, 0);
        int completedCount = m_dateToCompletedCount.value(date, 0);
        
        if (totalCount > 0) {
            QTextCharFormat format;
            
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
}