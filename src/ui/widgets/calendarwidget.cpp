#include "calendarwidget.h"
#include "../theme.h"
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
    
    // 玻璃单元格：半透明底透出极光背景，选中项霓虹光晕
    QColor bgColor = m_selected ? Theme::withAlpha(Theme::primary(), 46) : Theme::glassBg();
    if (m_otherMonth) {
        bgColor = Theme::isDark() ? QColor(255, 255, 255, 4) : QColor(255, 255, 255, 110);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);

    if (m_selected) {
        painter.setPen(QPen(Theme::primary(), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
    }

    // 今天：日期下方一颗霓虹小点
    if (m_today) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::primary());
        painter.drawEllipse(QPointF(rect().center().x(), getDateRect().bottom() + 2), 2.5, 2.5);
    }

    QFont dateFont;
    dateFont.setPixelSize(12);
    dateFont.setBold(m_today);
    painter.setFont(dateFont);

    QColor dateColor = m_otherMonth ? Theme::textMuted() :
                       m_today ? Theme::primaryHover() : Theme::textPrimary();
    painter.setPen(dateColor);

    QRect dateRect = getDateRect();
    painter.drawText(dateRect, Qt::AlignCenter, QString::number(m_date.day()));

    QList<QColor> priorityColors = {
        Theme::primary(),
        Theme::warning(),
        Theme::danger()
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
                tagColor = Theme::withAlpha(Theme::textDisabled(), 120);
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
        QColor textColor = todo.isCompleted() ? Theme::textMuted() : Theme::textPrimary();
        painter.setPen(textColor);
        
        QFontMetrics fm(todoFont);
        QString text = fm.elidedText(todo.getTitle(), Qt::ElideRight, todoRect.width() - 6);
        painter.drawText(todoRect.adjusted(3, 0, -3, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
    }
    
    if (m_todos.size() > 3) {
        QFont moreFont;
        moreFont.setPixelSize(9);
        painter.setFont(moreFont);
        painter.setPen(Theme::textSecondary());
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
    m_prevYearBtn->setFixedSize(36, 32);
    m_prevYearBtn->setCursor(Qt::PointingHandCursor);
    m_prevYearBtn->setToolTip("上一年");
    m_headerLayout->addWidget(m_prevYearBtn);

    m_prevBtn = new QPushButton("<");
    m_prevBtn->setFixedSize(36, 32);
    m_prevBtn->setCursor(Qt::PointingHandCursor);
    m_prevBtn->setToolTip("上一月");
    m_headerLayout->addWidget(m_prevBtn);

    m_monthLabel = new QLabel();
    m_monthLabel->setAlignment(Qt::AlignCenter);
    m_headerLayout->addWidget(m_monthLabel, 1);

    m_nextBtn = new QPushButton(">");
    m_nextBtn->setFixedSize(36, 32);
    m_nextBtn->setCursor(Qt::PointingHandCursor);
    m_nextBtn->setToolTip("下一月");
    m_headerLayout->addWidget(m_nextBtn);

    m_nextYearBtn = new QPushButton(">>");
    m_nextYearBtn->setFixedSize(36, 32);
    m_nextYearBtn->setCursor(Qt::PointingHandCursor);
    m_nextYearBtn->setToolTip("下一年");
    m_headerLayout->addWidget(m_nextYearBtn);

    m_mainLayout->addWidget(m_headerWidget);

    m_weekHeader = new QWidget();
    m_weekHeaderLayout = new QHBoxLayout(m_weekHeader);
    m_weekHeaderLayout->setContentsMargins(4, 8, 4, 8);
    m_weekHeaderLayout->setSpacing(2);

    QStringList weekDays = {"一", "二", "三", "四", "五", "六", "日"};
    for (const QString &day : weekDays) {
        QLabel *label = new QLabel(day);
        label->setAlignment(Qt::AlignCenter);
        label->setProperty("weekDayLabel", true);
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

    refreshTheme();
    updateCells();
}

void CalendarGrid::refreshTheme()
{
    const QString btnStyle = QStringLiteral(
        "QPushButton { background-color: %1; border: none; border-radius: 6px; "
        "color: %2; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: %3; }"
        "QPushButton:pressed { background-color: %4; }")
        .arg(Theme::primarySoft2().name(), Theme::primaryHover().name(),
             Theme::withAlpha(Theme::primary(), 60).name(QColor::HexArgb),
             Theme::withAlpha(Theme::primary(), 90).name(QColor::HexArgb));

    for (QPushButton *btn : {m_prevYearBtn, m_prevBtn, m_nextBtn, m_nextYearBtn}) {
        btn->setStyleSheet(btnStyle);
    }

    m_monthLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 16px; font-weight: 600;")
                                .arg(Theme::textPrimary().name()));
    m_weekHeader->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 6px;")
                                .arg(Theme::surfaceAlt().name()));

    const QString dayStyle = QStringLiteral("color: %1; font-size: 12px; font-weight: 600;")
                             .arg(Theme::textSecondary().name());
    for (QLabel *label : m_weekHeader->findChildren<QLabel*>()) {
        if (label->property("weekDayLabel").toBool()) {
            label->setStyleSheet(dayStyle);
        }
    }
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
            // 完成项底色加深，避免暗色下几乎看不见
            QColor lightColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(),
                                       Theme::isDark() ? 55 : 25);
            QColor whiteColor = Theme::surface();
            QLinearGradient gradient(contentRect.left(), contentRect.top(),
                                     contentRect.right(), contentRect.top());
            gradient.setColorAt(0, lightColor);
            gradient.setColorAt(1, whiteColor);
            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            painter.drawRoundedRect(contentRect, 6, 6);
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::isDark() ? Theme::glassBgStrong() : Theme::surface());
            painter.drawRoundedRect(contentRect, 6, 6);
        }
    } else {
        if (!m_tagColor.isEmpty()) {
            QColor baseColor(m_tagColor);
            QColor lightColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 40);
            QColor whiteColor = Theme::surface();
            QLinearGradient gradient(contentRect.left(), contentRect.top(),
                                     contentRect.right(), contentRect.top());
            gradient.setColorAt(0, lightColor);
            gradient.setColorAt(1, whiteColor);
            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            painter.drawRoundedRect(contentRect, 6, 6);
        } else {
            QColor bgColor = m_selected ? Theme::primarySoft() : Theme::surface();
            painter.setPen(Qt::NoPen);
            painter.setBrush(bgColor);
            painter.drawRoundedRect(contentRect, 6, 6);
        }
    }

    if (m_selected) {
        painter.setPen(QPen(Theme::borderStrong(), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(contentRect, 6, 6);
    }

    QColor tagColor = Theme::borderStrong();
    if (!m_tagColor.isEmpty()) {
        tagColor = QColor(m_tagColor);
        if (m_completed) {
            tagColor = QColor(tagColor.red(), tagColor.green(), tagColor.blue(),
                              Theme::isDark() ? 170 : 100);
        }
    }
    if (m_completed && m_tagColor.isEmpty()) {
        tagColor = Theme::textMuted();
    }
    painter.setBrush(tagColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(12, 10, 4, height() - 20, 2, 2);

    // 完成项：次级文字色 + 删除线，既清晰可读又有"已完成"语义
    QFont titleFont;
    titleFont.setPixelSize(13);
    titleFont.setBold(!m_completed);
    titleFont.setStrikeOut(m_completed);
    painter.setFont(titleFont);
    painter.setPen(m_completed ? Theme::textSecondary() : Theme::textPrimary());
    
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

    m_rightPanel->setMinimumWidth(280);
    m_rightPanel->setMaximumWidth(320);

    QWidget *headerWidget = new QWidget();
    headerWidget->setStyleSheet("background-color: transparent;");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 16, 20, 12);
    headerLayout->setSpacing(6);

    m_dateLabel = new QLabel();
    headerLayout->addWidget(m_dateLabel);

    m_countLabel = new QLabel();
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
    m_addLayout = new QHBoxLayout(m_addPanel);
    m_addLayout->setContentsMargins(16, 12, 16, 12);
    m_addLayout->setSpacing(8);

    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("添加待办事项...");
    m_addLayout->addWidget(m_addLineEdit, 1);

    m_addButton = new QPushButton("添加");
    m_addLayout->addWidget(m_addButton);

    m_rightLayout->addWidget(m_addPanel);

    QWidget *buttonPanel = new QWidget();
    buttonPanel->setStyleSheet("background-color: transparent;");
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonPanel);
    buttonLayout->setContentsMargins(16, 0, 16, 16);
    buttonLayout->setSpacing(8);

    m_toggleButton = new QPushButton("完成");
    m_toggleButton->setEnabled(false);
    buttonLayout->addWidget(m_toggleButton);

    m_deleteButton = new QPushButton("删除");
    m_deleteButton->setEnabled(false);
    buttonLayout->addWidget(m_deleteButton);

    m_rightLayout->addWidget(buttonPanel);

    m_mainLayout->addWidget(m_rightPanel);

    refreshTheme();
    updateDateLabel();
}

void CalendarWidget::refreshTheme()
{
    m_rightPanel->setStyleSheet(QStringLiteral(
        "background-color: %1; border: 1px solid %2; border-radius: 12px;")
        .arg(Theme::glassBg().name(QColor::HexArgb), Theme::glassBorder().name(QColor::HexArgb)));

    m_dateLabel->setStyleSheet(QStringLiteral(
        "font-size: 14px; font-weight: 600; color: %1; border: none;")
        .arg(Theme::textPrimary().name()));
    m_countLabel->setStyleSheet(QStringLiteral(
        "font-size: 12px; color: %1; border: none;")
        .arg(Theme::textSecondary().name()));

    m_addPanel->setStyleSheet(QStringLiteral(
        "background-color: transparent; border-top: 1px solid %1;")
        .arg(Theme::border().name()));

    m_addLineEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background-color: %1; border: 1px solid %2; border-radius: 8px; "
        "padding: 8px 12px; color: %3; font-size: 13px; }"
        "QLineEdit:focus { border-color: %4; background-color: %5; }"
        "QLineEdit::placeholder { color: %6; }")
        .arg(Theme::surfaceAlt().name(), Theme::border().name(), Theme::textPrimary().name(),
             Theme::textMuted().name(), Theme::surface().name(), Theme::textMuted().name()));

    // 彩色描边按钮：颜色语义固定（绿=添加 蓝=完成 橙=删除），仅背景/禁用态随主题
    const QString outlineBtn = QStringLiteral(
        "QPushButton { background-color: %1; border: 2px solid %2; border-radius: 8px; "
        "padding: 8px 16px; color: %2; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background-color: %3; }"
        "QPushButton:pressed { background-color: %4; }");
    const QString disabledSuffix = QStringLiteral(
        "QPushButton:disabled { background-color: %1; color: %2; border-color: %3; }")
        .arg(Theme::surface().name(), Theme::textMuted().name(), Theme::border().name());

    const QString green  = Theme::isDark() ? QStringLiteral("#4ade80") : QStringLiteral("#22c55e");
    const QString blue   = Theme::primary().name();
    const QString orange = Theme::warning().name();

    m_addButton->setStyleSheet(outlineBtn
        .arg(Theme::surface().name(), green,
             QStringLiteral("rgba(74, 222, 128, 0.12)"), QStringLiteral("rgba(74, 222, 128, 0.24)")));
    m_toggleButton->setStyleSheet(outlineBtn
        .arg(Theme::surface().name(), blue,
             Theme::withAlpha(Theme::primary(), 25).name(QColor::HexArgb),
             Theme::withAlpha(Theme::primary(), 50).name(QColor::HexArgb))
        + disabledSuffix);
    m_deleteButton->setStyleSheet(outlineBtn
        .arg(Theme::surface().name(), orange,
             Theme::withAlpha(Theme::warning(), 25).name(QColor::HexArgb),
             Theme::withAlpha(Theme::warning(), 50).name(QColor::HexArgb))
        + disabledSuffix);

    // 网格头部与单元格自绘色一并刷新
    m_calendarGrid->refreshTheme();
    m_calendarGrid->update();
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

