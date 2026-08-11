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
    
    QList<QColor> priorityColors = {
        QColor(59, 130, 246),
        QColor(245, 158, 11),
        QColor(239, 68, 68)
    };
    
    int maxTodos = qMin(3, m_todos.size());
    for (int i = 0; i < maxTodos; ++i) {
        QRect todoRect = getTodoRect(i);
        if (todoRect.isEmpty()) break;
        
        const TodoItem &todo = m_todos[i];
        
        QColor tagColor;
        if (todo.isCompleted()) {
            if (!todo.getTagColor().isEmpty()) {
                QColor baseColor(todo.getTagColor());
                tagColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 60);
            } else {
                tagColor = QColor(200, 205, 210, 120);
            }
        } else if (!todo.getTagColor().isEmpty()) {
            QColor baseColor(todo.getTagColor());
            tagColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 140);
        } else {
            int priority = qBound(0, todo.getPriority(), 2);
            QColor baseColor = priorityColors[priority];
            tagColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 140);
        }
        
        painter.setPen(Qt::NoPen);
        painter.setBrush(tagColor);
        painter.drawRoundedRect(todoRect, 3, 3);
        
        QFont todoFont;
        todoFont.setPixelSize(10);
        painter.setFont(todoFont);
        QColor textColor = todo.isCompleted() ? QColor(120, 125, 130) : QColor(50, 55, 60);
        painter.setPen(textColor);
        
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
    
    QString btnStyle = 
        "QPushButton { background-color: #dbeafe; border: none; border-radius: 6px; "
        "color: #1e40af; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #bfdbfe; }"
        "QPushButton:pressed { background-color: #93c5fd; }";
    
    m_prevYearBtn = new QPushButton("<<");
    m_prevYearBtn->setFixedSize(36, 32);
    m_prevYearBtn->setCursor(Qt::PointingHandCursor);
    m_prevYearBtn->setToolTip("上一年");
    m_prevYearBtn->setStyleSheet(btnStyle);
    m_headerLayout->addWidget(m_prevYearBtn);
    
    m_prevBtn = new QPushButton("<");
    m_prevBtn->setFixedSize(36, 32);
    m_prevBtn->setCursor(Qt::PointingHandCursor);
    m_prevBtn->setToolTip("上一月");
    m_prevBtn->setStyleSheet(btnStyle);
    m_headerLayout->addWidget(m_prevBtn);
    
    m_monthLabel = new QLabel();
    m_monthLabel->setStyleSheet("color: #1f2937; font-size: 16px; font-weight: 600;");
    m_monthLabel->setAlignment(Qt::AlignCenter);
    m_headerLayout->addWidget(m_monthLabel, 1);
    
    m_nextBtn = new QPushButton(">");
    m_nextBtn->setFixedSize(36, 32);
    m_nextBtn->setCursor(Qt::PointingHandCursor);
    m_nextBtn->setToolTip("下一月");
    m_nextBtn->setStyleSheet(btnStyle);
    m_headerLayout->addWidget(m_nextBtn);
    
    m_nextYearBtn = new QPushButton(">>");
    m_nextYearBtn->setFixedSize(36, 32);
    m_nextYearBtn->setCursor(Qt::PointingHandCursor);
    m_nextYearBtn->setToolTip("下一年");
    m_nextYearBtn->setStyleSheet(btnStyle);
    m_headerLayout->addWidget(m_nextYearBtn);
    
    m_mainLayout->addWidget(m_headerWidget);
    
    m_weekHeader = new QWidget();
    m_weekHeader->setStyleSheet("background-color: #f1f5f9; border-radius: 6px;");
    m_weekHeaderLayout = new QHBoxLayout(m_weekHeader);
    m_weekHeaderLayout->setContentsMargins(4, 8, 4, 8);
    m_weekHeaderLayout->setSpacing(2);
    
    QStringList weekDays = {"一", "二", "三", "四", "五", "六", "日"};
    for (const QString &day : weekDays) {
        QLabel *label = new QLabel(day);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #64748b; font-size: 12px; font-weight: 600;");
        m_weekHeaderLayout->addWidget(label);
    }
    m_mainLayout->addWidget(m_weekHeader);
    
    m_gridWidget = new QWidget();
    m_gridLayout = new QGridLayout(m_gridWidget);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(2);
    
    for (int row = 0; row < 6; ++row) {
        m_gridLayout->setRowMinimumHeight(row, 80);
        for (int col = 0; col < 7; ++col) {
            CalendarCell *cell = new CalendarCell();
            m_cells.append(cell);
            m_gridLayout->addWidget(cell, row, col);
            m_gridLayout->setRowStretch(row, 1);
            m_gridLayout->setColumnStretch(col, 1);
            connect(cell, &CalendarCell::clicked, this, &CalendarGrid::dateClicked);
            connect(cell, &CalendarCell::todoClicked, this, &CalendarGrid::todoClicked);
        }
    }
    
    m_mainLayout->addWidget(m_gridWidget, 1);
    
    connect(m_prevBtn, &QPushButton::clicked, this, &CalendarGrid::onPrevMonth);
    connect(m_nextBtn, &QPushButton::clicked, this, &CalendarGrid::onNextMonth);
    connect(m_prevYearBtn, &QPushButton::clicked, this, &CalendarGrid::onPrevYear);
    connect(m_nextYearBtn, &QPushButton::clicked, this, &CalendarGrid::onNextYear);
    
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
    
    QRect contentRect = rect().adjusted(4, 2, -4, -2);
    
    if (m_completed) {
        if (!m_tagColor.isEmpty()) {
            QColor baseColor(m_tagColor);
            QColor lightColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 25);
            QColor whiteColor = QColor(255, 255, 255, 255);
            QLinearGradient gradient(contentRect.left(), contentRect.top(), 
                                     contentRect.right(), contentRect.top());
            gradient.setColorAt(0, lightColor);
            gradient.setColorAt(1, whiteColor);
            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            painter.drawRoundedRect(contentRect, 6, 6);
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(252, 252, 253));
            painter.drawRoundedRect(contentRect, 6, 6);
        }
    } else {
        if (!m_tagColor.isEmpty()) {
            QColor baseColor(m_tagColor);
            QColor lightColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 40);
            QColor whiteColor = QColor(255, 255, 255, 255);
            QLinearGradient gradient(contentRect.left(), contentRect.top(), 
                                     contentRect.right(), contentRect.top());
            gradient.setColorAt(0, lightColor);
            gradient.setColorAt(1, whiteColor);
            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            painter.drawRoundedRect(contentRect, 6, 6);
        } else {
            QColor bgColor = m_selected ? QColor(239, 246, 255) : QColor(255, 255, 255);
            painter.setPen(Qt::NoPen);
            painter.setBrush(bgColor);
            painter.drawRoundedRect(contentRect, 6, 6);
        }
    }
    
    if (m_selected) {
        painter.setPen(QPen(QColor(148, 163, 184), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(contentRect, 6, 6);
    }
    
    QColor tagColor = QColor(148, 163, 184);
    if (!m_tagColor.isEmpty()) {
        tagColor = QColor(m_tagColor);
        if (m_completed) {
            tagColor = QColor(tagColor.red(), tagColor.green(), tagColor.blue(), 100);
        }
    }
    if (m_completed && m_tagColor.isEmpty()) {
        tagColor = QColor(200, 205, 210);
    }
    painter.setBrush(tagColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(12, 10, 4, height() - 20, 2, 2);
    
    QFont titleFont;
    titleFont.setPixelSize(13);
    titleFont.setBold(!m_completed);
    painter.setFont(titleFont);
    painter.setPen(m_completed ? QColor(180, 185, 190) : QColor(31, 41, 55));
    
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
    
    m_calendarGrid = new CalendarGrid();
    m_leftLayout->addWidget(m_calendarGrid, 1);
    
    m_mainLayout->addWidget(m_leftPanel, 2);
    
    m_rightPanel = new QWidget();
    m_rightLayout = new QVBoxLayout(m_rightPanel);
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_rightLayout->setSpacing(12);
    
    m_rightPanel->setStyleSheet("background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px;");
    m_rightPanel->setMinimumWidth(280);
    m_rightPanel->setMaximumWidth(320);
    
    QWidget *headerWidget = new QWidget();
    headerWidget->setStyleSheet("background-color: transparent;");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 16, 20, 12);
    headerLayout->setSpacing(6);
    
    m_dateLabel = new QLabel();
    m_dateLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #1e293b; border: none;");
    headerLayout->addWidget(m_dateLabel);
    
    m_countLabel = new QLabel();
    m_countLabel->setStyleSheet("font-size: 12px; color: #64748b; border: none;");
    headerLayout->addWidget(m_countLabel);
    
    m_rightLayout->addWidget(headerWidget);
    
    m_todoScrollArea = new QScrollArea();
    m_todoScrollArea->setWidgetResizable(true);
    m_todoScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_todoScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_todoScrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");
    
    m_todoContainer = new QWidget();
    m_todoContainer->setStyleSheet("background-color: transparent;");
    m_todoListLayout = new QVBoxLayout(m_todoContainer);
    m_todoListLayout->setContentsMargins(12, 12, 12, 12);
    m_todoListLayout->setSpacing(6);
    m_todoListLayout->addStretch();
    
    m_todoScrollArea->setWidget(m_todoContainer);
    m_rightLayout->addWidget(m_todoScrollArea, 1);
    
    m_addPanel = new QWidget();
    m_addPanel->setStyleSheet("background-color: transparent; border-top: 1px solid #f1f5f9;");
    m_addLayout = new QHBoxLayout(m_addPanel);
    m_addLayout->setContentsMargins(16, 12, 16, 12);
    m_addLayout->setSpacing(8);
    
    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("添加待办事项...");
    m_addLineEdit->setStyleSheet(
        "QLineEdit { background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; "
        "padding: 8px 12px; color: #334155; font-size: 13px; }"
        "QLineEdit:focus { border-color: #94a3b8; background-color: #ffffff; }"
        "QLineEdit::placeholder { color: #94a3b8; }"
    );
    m_addLayout->addWidget(m_addLineEdit, 1);
    
    QString btnStyle = 
        "QPushButton { background-color: #ffffff; border: 2px solid #22c55e; border-radius: 8px; "
        "padding: 8px 16px; color: #22c55e; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background-color: rgba(34, 197, 94, 0.1); }"
        "QPushButton:pressed { background-color: rgba(34, 197, 94, 0.2); }";
    
    m_addButton = new QPushButton("添加");
    m_addButton->setStyleSheet(btnStyle);
    m_addLayout->addWidget(m_addButton);
    
    m_rightLayout->addWidget(m_addPanel);
    
    QWidget *buttonPanel = new QWidget();
    buttonPanel->setStyleSheet("background-color: transparent;");
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonPanel);
    buttonLayout->setContentsMargins(16, 0, 16, 16);
    buttonLayout->setSpacing(8);
    
    m_toggleButton = new QPushButton("完成");
    m_toggleButton->setStyleSheet(
        "QPushButton { background-color: #ffffff; border: 2px solid #3b82f6; border-radius: 8px; "
        "padding: 8px 16px; color: #3b82f6; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background-color: rgba(59, 130, 246, 0.1); }"
        "QPushButton:pressed { background-color: rgba(59, 130, 246, 0.2); }"
        "QPushButton:disabled { background-color: #ffffff; color: #94a3b8; border-color: #e2e8f0; }"
    );
    m_toggleButton->setEnabled(false);
    buttonLayout->addWidget(m_toggleButton);
    
    m_deleteButton = new QPushButton("删除");
    m_deleteButton->setStyleSheet(
        "QPushButton { background-color: #ffffff; border: 2px solid #f59e0b; border-radius: 8px; "
        "padding: 8px 16px; color: #f59e0b; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background-color: rgba(245, 158, 11, 0.1); }"
        "QPushButton:pressed { background-color: rgba(245, 158, 11, 0.2); }"
        "QPushButton:disabled { background-color: #ffffff; color: #94a3b8; border-color: #e2e8f0; }"
    );
    m_deleteButton->setEnabled(false);
    buttonLayout->addWidget(m_deleteButton);
    
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
            QDate dueDate = item.getDueDate();
            if (dueDate.isValid()) {
                m_dateToTodos[dueDate].append(item);
            }
        }
    }
    
    m_calendarGrid->setTodoData(m_dateToTodos);
}

void CalendarWidget::refreshTodoList()
{
    for (TodoListItem *item : m_todoItems) {
        item->deleteLater();
    }
    m_todoItems.clear();
    
    QList<TodoItem> todos;
    if (m_dateToTodos.contains(m_currentDate)) {
        todos = m_dateToTodos[m_currentDate];
    }
    
    std::sort(todos.begin(), todos.end(), [](const TodoItem &a, const TodoItem &b) {
        if (a.isCompleted() != b.isCompleted()) {
            return a.isCompleted() < b.isCompleted();
        }
        return a.getCreatedTime() > b.getCreatedTime();
    });
    
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

