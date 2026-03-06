#include "calendarwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QScrollBar>
#include <QStyle>

CalendarCell::CalendarCell(QWidget *parent)
    : QWidget(parent)
    , m_otherMonth(false)
    , m_selected(false)
    , m_today(false)
    , m_pressed(false)
    , m_clickedTodoIndex(-1)
{
    setMinimumSize(80, 80);
    setCursor(Qt::PointingHandCursor);
}

void CalendarCell::setDate(const QDate &date)
{
    m_date = date;
    update();
}

void CalendarCell::setTodos(const QList<TodoItem> &todos)
{
    m_todos = todos;
    update();
}

void CalendarCell::setOtherMonth(bool other)
{
    m_otherMonth = other;
    update();
}

void CalendarCell::setSelected(bool selected)
{
    m_selected = selected;
    update();
}

void CalendarCell::setToday(bool today)
{
    m_today = today;
    update();
}

QRect CalendarCell::getDateRect() const
{
    return QRect(4, 4, width() - 8, 20);
}

QRect CalendarCell::getTodoRect(int index) const
{
    int todoHeight = 16;
    int todoTop = 26 + index * (todoHeight + 2);
    if (todoTop + todoHeight > height() - 4) {
        return QRect();
    }
    return QRect(4, todoTop, width() - 8, todoHeight);
}

void CalendarCell::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor bgColor = m_selected ? QColor(239, 246, 255) : QColor(255, 255, 255);
    if (m_otherMonth) {
        bgColor = QColor(249, 250, 251);
    }
    painter.fillRect(rect(), bgColor);
    
    if (m_selected) {
        painter.setPen(QPen(QColor(37, 99, 235), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
    }
    
    QFont dateFont;
    dateFont.setPixelSize(12);
    dateFont.setBold(m_today);
    painter.setFont(dateFont);
    
    QColor dateColor = m_otherMonth ? QColor(156, 163, 175) : 
                       m_today ? QColor(37, 99, 235) : QColor(55, 65, 81);
    painter.setPen(dateColor);
    
    QRect dateRect = getDateRect();
    painter.drawText(dateRect, Qt::AlignCenter, QString::number(m_date.day()));
    
    int maxTodos = qMin(3, m_todos.size());
    for (int i = 0; i < maxTodos; ++i) {
        QRect todoRect = getTodoRect(i);
        if (todoRect.isEmpty()) break;
        
        const TodoItem &todo = m_todos[i];
        
        QColor tagColor(37, 99, 235);
        if (!todo.getTagColor().isEmpty()) {
            tagColor = QColor(todo.getTagColor());
        }
        
        painter.setPen(Qt::NoPen);
        painter.setBrush(tagColor);
        painter.drawRoundedRect(todoRect, 3, 3);
        
        QFont todoFont;
        todoFont.setPixelSize(10);
        painter.setFont(todoFont);
        painter.setPen(Qt::white);
        
        QFontMetrics fm(todoFont);
        QString text = fm.elidedText(todo.getTitle(), Qt::ElideRight, todoRect.width() - 6);
        painter.drawText(todoRect.adjusted(3, 0, -3, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
    }
    
    if (m_todos.size() > 3) {
        QFont moreFont;
        moreFont.setPixelSize(9);
        painter.setFont(moreFont);
        painter.setPen(QColor(107, 114, 128));
        QString moreText = QString("+%1").arg(m_todos.size() - 3);
        QRect moreRect = getTodoRect(3);
        if (!moreRect.isEmpty()) {
            painter.drawText(moreRect, Qt::AlignLeft | Qt::AlignVCenter, moreText);
        }
    }
}

void CalendarCell::mousePressEvent(QMouseEvent *event)
{
    m_pressed = true;
    m_clickedTodoIndex = -1;
    
    for (int i = 0; i < qMin(3, m_todos.size()); ++i) {
        QRect todoRect = getTodoRect(i);
        if (todoRect.contains(event->pos())) {
            m_clickedTodoIndex = i;
            break;
        }
    }
    update();
}

void CalendarCell::mouseReleaseEvent(QMouseEvent *event)
{
    m_pressed = false;
    
    if (m_clickedTodoIndex >= 0 && m_clickedTodoIndex < m_todos.size()) {
        if (getTodoRect(m_clickedTodoIndex).contains(event->pos())) {
            emit todoClicked(m_todos[m_clickedTodoIndex].getId());
        }
    } else {
        emit clicked(m_date);
    }
    m_clickedTodoIndex = -1;
    update();
}

CalendarGrid::CalendarGrid(QWidget *parent)
    : QWidget(parent)
    , m_year(QDate::currentDate().year())
    , m_month(QDate::currentDate().month())
    , m_selectedDate(QDate::currentDate())
{
    setupUI();
}

void CalendarGrid::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(8);
    
    m_headerWidget = new QWidget();
    m_headerLayout = new QHBoxLayout(m_headerWidget);
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(4);
    
    m_prevYearBtn = new QPushButton("<<");
    m_prevYearBtn->setFixedSize(32, 32);
    m_prevYearBtn->setCursor(Qt::PointingHandCursor);
    m_prevYearBtn->setToolTip("上一年");
    m_headerLayout->addWidget(m_prevYearBtn);
    
    m_prevBtn = new QPushButton("<");
    m_prevBtn->setFixedSize(32, 32);
    m_prevBtn->setCursor(Qt::PointingHandCursor);
    m_prevBtn->setToolTip("上一月");
    m_headerLayout->addWidget(m_prevBtn);
    
    m_monthLabel = new QLabel();
    m_monthLabel->setStyleSheet("color: #1f2937; font-size: 16px; font-weight: 600;");
    m_monthLabel->setAlignment(Qt::AlignCenter);
    m_headerLayout->addWidget(m_monthLabel, 1);
    
    m_nextBtn = new QPushButton(">");
    m_nextBtn->setFixedSize(32, 32);
    m_nextBtn->setCursor(Qt::PointingHandCursor);
    m_nextBtn->setToolTip("下一月");
    m_headerLayout->addWidget(m_nextBtn);
    
    m_nextYearBtn = new QPushButton(">>");
    m_nextYearBtn->setFixedSize(32, 32);
    m_nextYearBtn->setCursor(Qt::PointingHandCursor);
    m_nextYearBtn->setToolTip("下一年");
    m_headerLayout->addWidget(m_nextYearBtn);
    
    m_mainLayout->addWidget(m_headerWidget);
    
    m_weekHeader = new QWidget();
    m_weekHeader->setStyleSheet("background-color: #e0e7ff; border-radius: 6px;");
    m_weekHeaderLayout = new QHBoxLayout(m_weekHeader);
    m_weekHeaderLayout->setContentsMargins(4, 8, 4, 8);
    m_weekHeaderLayout->setSpacing(2);
    
    QStringList weekDays = {"一", "二", "三", "四", "五", "六", "日"};
    for (const QString &day : weekDays) {
        QLabel *label = new QLabel(day);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #3730a3; font-size: 12px; font-weight: 600;");
        m_weekHeaderLayout->addWidget(label);
    }
    m_mainLayout->addWidget(m_weekHeader);
    
    m_gridWidget = new QWidget();
    m_gridLayout = new QGridLayout(m_gridWidget);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(2);
    
    for (int i = 0; i < 42; ++i) {
        CalendarCell *cell = new CalendarCell();
        m_cells.append(cell);
        m_gridLayout->addWidget(cell, i / 7, i % 7);
        connect(cell, &CalendarCell::clicked, this, &CalendarGrid::dateClicked);
        connect(cell, &CalendarCell::todoClicked, this, &CalendarGrid::todoClicked);
    }
    
    m_mainLayout->addWidget(m_gridWidget);
    
    connect(m_prevYearBtn, &QPushButton::clicked, this, &CalendarGrid::onPrevYear);
    connect(m_nextYearBtn, &QPushButton::clicked, this, &CalendarGrid::onNextYear);
    connect(m_prevBtn, &QPushButton::clicked, this, &CalendarGrid::onPrevMonth);
    connect(m_nextBtn, &QPushButton::clicked, this, &CalendarGrid::onNextMonth);
    
    updateCells();
}

bool CalendarGrid::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)
    Q_UNUSED(event)
    return false;
}

void CalendarGrid::setCurrentMonth(int year, int month)
{
    m_year = year;
    m_month = month;
    updateCells();
    emit monthChanged(year, month);
}

void CalendarGrid::setTodoData(const QMap<QDate, QList<TodoItem>> &todos)
{
    m_todoData = todos;
    updateCells();
}

void CalendarGrid::setSelectedDate(const QDate &date)
{
    m_selectedDate = date;
    updateCells();
}

void CalendarGrid::updateCells()
{
    m_monthLabel->setText(QString("%1年%2月").arg(m_year).arg(m_month));
    
    QDate firstDay(m_year, m_month, 1);
    int startDayOfWeek = firstDay.dayOfWeek();
    QDate startDate = firstDay.addDays(-(startDayOfWeek - 1));
    
    QDate today = QDate::currentDate();
    
    for (int i = 0; i < 42; ++i) {
        QDate cellDate = startDate.addDays(i);
        CalendarCell *cell = m_cells[i];
        
        cell->setDate(cellDate);
        cell->setOtherMonth(cellDate.month() != m_month);
        cell->setSelected(cellDate == m_selectedDate);
        cell->setToday(cellDate == today);
        
        QList<TodoItem> cellTodos;
        if (m_todoData.contains(cellDate)) {
            cellTodos = m_todoData[cellDate];
        }
        cell->setTodos(cellTodos);
    }
}

void CalendarGrid::onPrevMonth()
{
    m_month--;
    if (m_month < 1) {
        m_month = 12;
        m_year--;
    }
    updateCells();
    emit monthChanged(m_year, m_month);
}

void CalendarGrid::onNextMonth()
{
    m_month++;
    if (m_month > 12) {
        m_month = 1;
        m_year++;
    }
    updateCells();
    emit monthChanged(m_year, m_month);
}

void CalendarGrid::onPrevYear()
{
    m_year--;
    updateCells();
    emit monthChanged(m_year, m_month);
}

void CalendarGrid::onNextYear()
{
    m_year++;
    updateCells();
    emit monthChanged(m_year, m_month);
}

TodoListItem::TodoListItem(const TodoItem &item, QWidget *parent)
    : QWidget(parent)
    , m_todoId(item.getId())
    , m_title(item.getTitle())
    , m_completed(item.isCompleted())
    , m_tagColor(item.getTagColor())
    , m_selected(false)
{
    setMinimumHeight(48);
    setCursor(Qt::PointingHandCursor);
}

void TodoListItem::setSelected(bool selected)
{
    m_selected = selected;
    update();
}

void TodoListItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_todoId);
    }
}

void TodoListItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(m_todoId);
    }
}

void TodoListItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor bgColor = m_selected ? QColor(239, 246, 255) : QColor(255, 255, 255);
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect().adjusted(4, 2, -4, -2), 6, 6);
    
    if (m_selected) {
        painter.setPen(QPen(QColor(37, 99, 235), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(4, 2, -4, -2), 6, 6);
    }
    
    QColor tagColor = QColor(37, 99, 235);
    if (!m_tagColor.isEmpty()) {
        tagColor = QColor(m_tagColor);
    }
    painter.setBrush(tagColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(12, 10, 4, height() - 20, 2, 2);
    
    QFont titleFont;
    titleFont.setPixelSize(13);
    if (m_completed) {
        titleFont.setStrikeOut(true);
    }
    painter.setFont(titleFont);
    painter.setPen(m_completed ? QColor(156, 163, 175) : QColor(31, 41, 55));
    
    QFontMetrics fm(titleFont);
    QString elidedTitle = fm.elidedText(m_title, Qt::ElideRight, width() - 24);
    painter.drawText(QRect(24, 0, width() - 32, height()), Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);
}

CalendarWidget::CalendarWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentDate(QDate::currentDate())
{
    setupUI();
    setupConnections();
}

CalendarWidget::~CalendarWidget()
{
}

void CalendarWidget::setupUI()
{
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(16, 16, 16, 16);
    m_mainLayout->setSpacing(16);
    
    m_leftPanel = new QWidget();
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    m_leftLayout->setSpacing(8);
    
    QWidget *yearMonthPanel = new QWidget();
    QHBoxLayout *yearMonthLayout = new QHBoxLayout(yearMonthPanel);
    yearMonthLayout->setContentsMargins(0, 0, 0, 0);
    yearMonthLayout->setSpacing(8);
    
    QPushButton *prevYearBtn = new QPushButton("<<");
    prevYearBtn->setFixedSize(40, 28);
    prevYearBtn->setCursor(Qt::PointingHandCursor);
    yearMonthLayout->addWidget(prevYearBtn);
    connect(prevYearBtn, &QPushButton::clicked, this, &CalendarWidget::onPrevYear);
    
    m_calendarGrid = new CalendarGrid();
    yearMonthLayout->addStretch();
    yearMonthLayout->addWidget(new QLabel(""));
    yearMonthLayout->addStretch();
    
    QPushButton *nextYearBtn = new QPushButton(">>");
    nextYearBtn->setFixedSize(40, 28);
    nextYearBtn->setCursor(Qt::PointingHandCursor);
    yearMonthLayout->addWidget(nextYearBtn);
    connect(nextYearBtn, &QPushButton::clicked, this, &CalendarWidget::onNextYear);
    
    m_leftLayout->addWidget(yearMonthPanel);
    m_leftLayout->addWidget(m_calendarGrid, 1);
    
    m_mainLayout->addWidget(m_leftPanel, 2);
    
    m_rightPanel = new QWidget();
    m_rightLayout = new QVBoxLayout(m_rightPanel);
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_rightLayout->setSpacing(12);
    
    m_rightPanel->setStyleSheet("background-color: #ffffff; border: 1px solid #e5e7eb; border-radius: 8px;");
    m_rightPanel->setMinimumWidth(280);
    m_rightPanel->setMaximumWidth(320);
    
    QWidget *headerWidget = new QWidget();
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(16, 16, 16, 8);
    headerLayout->setSpacing(4);
    
    m_dateLabel = new QLabel();
    m_dateLabel->setStyleSheet("font-size: 15px; font-weight: 600; color: #1f2937;");
    headerLayout->addWidget(m_dateLabel);
    
    m_countLabel = new QLabel();
    m_countLabel->setStyleSheet("font-size: 12px; color: #6b7280;");
    headerLayout->addWidget(m_countLabel);
    
    m_rightLayout->addWidget(headerWidget);
    
    m_todoScrollArea = new QScrollArea();
    m_todoScrollArea->setWidgetResizable(true);
    m_todoScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_todoScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_todoScrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");
    
    m_todoContainer = new QWidget();
    m_todoListLayout = new QVBoxLayout(m_todoContainer);
    m_todoListLayout->setContentsMargins(8, 0, 8, 0);
    m_todoListLayout->setSpacing(4);
    m_todoListLayout->addStretch();
    
    m_todoScrollArea->setWidget(m_todoContainer);
    m_rightLayout->addWidget(m_todoScrollArea, 1);
    
    m_addPanel = new QWidget();
    m_addLayout = new QHBoxLayout(m_addPanel);
    m_addLayout->setContentsMargins(16, 8, 16, 16);
    m_addLayout->setSpacing(8);
    
    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("添加待办事项...");
    m_addLayout->addWidget(m_addLineEdit, 1);
    
    m_addButton = new QPushButton("添加");
    m_addButton->setObjectName("newTodoBtn");
    m_addLayout->addWidget(m_addButton);
    
    m_rightLayout->addWidget(m_addPanel);
    
    QWidget *buttonPanel = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonPanel);
    buttonLayout->setContentsMargins(16, 0, 16, 16);
    buttonLayout->setSpacing(8);
    
    m_toggleButton = new QPushButton("完成");
    m_toggleButton->setObjectName("newTodoBtn");
    m_toggleButton->setEnabled(false);
    buttonLayout->addWidget(m_toggleButton);
    
    m_deleteButton = new QPushButton("删除");
    m_deleteButton->setObjectName("deleteBtn");
    m_deleteButton->setEnabled(false);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    
    m_rightLayout->addWidget(buttonPanel);
    
    m_mainLayout->addWidget(m_rightPanel);
    
    updateDateLabel();
}

void CalendarWidget::setupConnections()
{
    connect(m_calendarGrid, &CalendarGrid::dateClicked, this, &CalendarWidget::onDateClicked);
    connect(m_calendarGrid, &CalendarGrid::todoClicked, this, &CalendarWidget::onTodoClicked);
    connect(m_calendarGrid, &CalendarGrid::monthChanged, [this](int, int) {
        updateDateLabel();
    });
    connect(m_addButton, &QPushButton::clicked, this, &CalendarWidget::onAddTodo);
    connect(m_deleteButton, &QPushButton::clicked, this, &CalendarWidget::onDeleteTodo);
    connect(m_toggleButton, &QPushButton::clicked, this, &CalendarWidget::onToggleTodo);
}

void CalendarWidget::updateDateLabel()
{
    m_dateLabel->setText(m_currentDate.toString("yyyy年MM月dd日 dddd"));
}

void CalendarWidget::updateTodoData(const QList<TodoFolder> &folders)
{
    m_folders = folders;
    refreshCalendarData();
    refreshTodoList();
}

void CalendarWidget::refreshCalendarData()
{
    m_dateToTodos.clear();
    
    for (const TodoFolder &folder : m_folders) {
        for (const TodoItem &item : folder.getItems()) {
            QDate date = item.getPlannedDate();
            if (date.isValid()) {
                m_dateToTodos[date].append(item);
            }
        }
    }
    
    m_calendarGrid->setTodoData(m_dateToTodos);
}

void CalendarWidget::refreshTodoList()
{
    for (TodoListItem *item : m_todoItems) {
        delete item;
    }
    m_todoItems.clear();
    
    QList<TodoItem> todos;
    if (m_dateToTodos.contains(m_currentDate)) {
        todos = m_dateToTodos[m_currentDate];
    }
    
    int completedCount = 0;
    for (const TodoItem &todo : todos) {
        if (todo.isCompleted()) {
            completedCount++;
        }
    }
    
    m_countLabel->setText(QString("共 %1 项，已完成 %2 项").arg(todos.size()).arg(completedCount));
    
    for (const TodoItem &todo : todos) {
        TodoListItem *item = new TodoListItem(todo);
        connect(item, &TodoListItem::clicked, this, &CalendarWidget::onTodoClicked);
        connect(item, &TodoListItem::doubleClicked, this, &CalendarWidget::onTodoDoubleClicked);
        m_todoListLayout->insertWidget(m_todoListLayout->count() - 1, item);
        m_todoItems.append(item);
    }
}

void CalendarWidget::onDateClicked(const QDate &date)
{
    m_currentDate = date;
    m_calendarGrid->setSelectedDate(date);
    updateDateLabel();
    refreshTodoList();
    m_selectedTodoId.clear();
    m_deleteButton->setEnabled(false);
    m_toggleButton->setEnabled(false);
}

void CalendarWidget::onTodoClicked(const QString &todoId)
{
    m_selectedTodoId = todoId;
    m_deleteButton->setEnabled(true);
    m_toggleButton->setEnabled(true);
    
    for (TodoListItem *item : m_todoItems) {
        item->setSelected(item->getTodoId() == todoId);
    }
}

void CalendarWidget::onTodoDoubleClicked(const QString &todoId)
{
    Q_UNUSED(todoId)
}

void CalendarWidget::onAddTodo()
{
    QString title = m_addLineEdit->text().trimmed();
    if (title.isEmpty()) {
        return;
    }
    
    emit todoItemAdded(title, m_currentDate);
    m_addLineEdit->clear();
}

void CalendarWidget::onDeleteTodo()
{
    if (m_selectedTodoId.isEmpty()) {
        return;
    }
    
    emit todoItemDeleted(m_selectedTodoId);
    m_selectedTodoId.clear();
    m_deleteButton->setEnabled(false);
    m_toggleButton->setEnabled(false);
}

void CalendarWidget::onToggleTodo()
{
    if (m_selectedTodoId.isEmpty()) {
        return;
    }
    
    for (const TodoFolder &folder : m_folders) {
        const TodoItem *item = nullptr;
        for (const TodoItem &i : folder.getItems()) {
            if (i.getId() == m_selectedTodoId) {
                item = &i;
                break;
            }
        }
        if (item) {
            emit todoItemToggled(m_selectedTodoId, !item->isCompleted());
            break;
        }
    }
}

void CalendarWidget::onPrevMonth()
{
}

void CalendarWidget::onNextMonth()
{
}

void CalendarWidget::onPrevYear()
{
    int year = m_calendarGrid->getYear() - 1;
    int month = m_calendarGrid->getMonth();
    m_calendarGrid->setCurrentMonth(year, month);
}

void CalendarWidget::onNextYear()
{
    int year = m_calendarGrid->getYear() + 1;
    int month = m_calendarGrid->getMonth();
    m_calendarGrid->setCurrentMonth(year, month);
}
