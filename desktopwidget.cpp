#include "desktopwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QCloseEvent>
#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QMenu>

DesktopWidget::DesktopWidget(QWidget *parent)
    : QWidget(parent)
    , m_dragging(false)
{
    m_folders.clear();
    
    setupUI();
    
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(60000);
    
    setupConnections();
    
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(300, 420);
    
    loadWindowPosition();
    
    m_refreshTimer->start();
    
    applyStyles();
    loadPendingItems();
}

DesktopWidget::~DesktopWidget()
{
    saveWindowPosition();
}

void DesktopWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(16, 16, 16, 16);
    m_mainLayout->setSpacing(12);
    
    m_headerWidget = new QWidget();
    m_headerLayout = new QHBoxLayout(m_headerWidget);
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(8);
    
    m_titleLabel = new QLabel("待办事项");
    m_titleLabel->setStyleSheet("font-weight: 600; font-size: 15px; color: #1e293b;");
    m_headerLayout->addWidget(m_titleLabel);
    
    m_countLabel = new QLabel();
    m_countLabel->setStyleSheet("font-size: 12px; color: #6366f1; font-weight: 500;");
    m_headerLayout->addWidget(m_countLabel);
    
    m_headerLayout->addStretch();
    m_mainLayout->addWidget(m_headerWidget);
    
    m_todoListWidget = new QListWidget();
    m_todoListWidget->setMinimumHeight(260);
    m_todoListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_mainLayout->addWidget(m_todoListWidget);
    
    m_addWidget = new QWidget();
    m_addLayout = new QHBoxLayout(m_addWidget);
    m_addLayout->setContentsMargins(0, 0, 0, 0);
    m_addLayout->setSpacing(8);
    
    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("快速添加...");
    m_addLayout->addWidget(m_addLineEdit);
    
    m_addButton = new QPushButton("添加");
    m_addButton->setFixedWidth(60);
    m_addLayout->addWidget(m_addButton);
    
    m_mainLayout->addWidget(m_addWidget);
}

void DesktopWidget::setupConnections()
{
    connect(m_addButton, &QPushButton::clicked, this, &DesktopWidget::onAddTodoClicked);
    connect(m_addLineEdit, &QLineEdit::returnPressed, this, &DesktopWidget::onAddTodoClicked);
    connect(m_todoListWidget, &QListWidget::itemClicked, this, &DesktopWidget::onTodoItemClicked);
    connect(m_refreshTimer, &QTimer::timeout, this, &DesktopWidget::onRefreshTimer);
}

void DesktopWidget::applyStyles()
{
    QString listStyle = 
        "QListWidget { border: none; background-color: transparent; }"
        "QListWidget::item { padding: 10px 12px; border-radius: 6px; margin: 2px 0; }"
        "QListWidget::item:hover { background-color: rgba(99, 102, 241, 0.08); }"
        "QListWidget::item:selected { background-color: rgba(99, 102, 241, 0.15); }";
    
    m_todoListWidget->setStyleSheet(listStyle);
    
    m_addLineEdit->setStyleSheet(
        "QLineEdit { padding: 10px 14px; border: 1px solid #e2e8f0; border-radius: 8px; background-color: #ffffff; }"
        "QLineEdit:focus { border-color: #6366f1; }"
    );
    
    m_addButton->setStyleSheet(
        "QPushButton { background-color: #6366f1; color: white; border: none; border-radius: 8px; padding: 10px 16px; font-weight: 500; }"
        "QPushButton:hover { background-color: #4f46e5; }"
    );
}

void DesktopWidget::updateTodoData(const QList<TodoFolder> &folders)
{
    m_folders = folders;
    loadPendingItems();
    updateTodoList();
}

void DesktopWidget::loadPendingItems()
{
    m_displayItems.clear();
    
    int count = 0;
    for (const TodoFolder &folder : m_folders) {
        QList<TodoItem> items = folder.getItems();
        for (const TodoItem &item : items) {
            if (!item.isCompleted() && count < 10) {
                m_displayItems.append(item);
                count++;
            }
        }
        if (count >= 10) break;
    }
}

void DesktopWidget::updateTodoList()
{
    m_todoListWidget->clear();
    
    for (const TodoItem &item : m_displayItems) {
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
    
    int pendingCount = 0;
    for (const TodoItem &item : m_displayItems) {
        if (!item.isCompleted()) pendingCount++;
    }
    
    m_countLabel->setText(QString("(%1)").arg(pendingCount));
}

void DesktopWidget::refreshDisplay()
{
    loadPendingItems();
    updateTodoList();
}

void DesktopWidget::onAddTodoClicked()
{
    QString title = m_addLineEdit->text().trimmed();
    if (!title.isEmpty()) {
        emit newTodoRequested(title);
        m_addLineEdit->clear();
    }
}

void DesktopWidget::onTodoItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    m_todoListWidget->setCurrentItem(item);
}

void DesktopWidget::onRefreshTimer()
{
    refreshDisplay();
}

void DesktopWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.setBrush(QBrush(QColor(255, 255, 255, 245)));
    painter.setPen(QPen(QColor(226, 232, 240), 1));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 16, 16);
    
    QWidget::paintEvent(event);
}

void DesktopWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        m_dragging = true;
        event->accept();
    }
}

void DesktopWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_dragging) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void DesktopWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu contextMenu(this);
    contextMenu.setStyleSheet(
        "QMenu { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 8px 24px; border-radius: 4px; color: #334155; }"
        "QMenu::item:selected { background-color: #eef2ff; color: #4f46e5; }"
    );
    
    QListWidgetItem *clickedItem = m_todoListWidget->itemAt(m_todoListWidget->mapFromParent(event->pos()));
    if (clickedItem) {
        QString itemId = clickedItem->data(Qt::UserRole).toString();
        
        bool isCompleted = false;
        for (const TodoItem &todo : m_displayItems) {
            if (todo.getId() == itemId) {
                isCompleted = todo.isCompleted();
                break;
            }
        }
        
        QString toggleText = isCompleted ? "标记为未完成" : "标记为已完成";
        contextMenu.addAction(toggleText, this, [this, itemId, isCompleted]() {
            emit todoItemToggled(itemId, !isCompleted);
        });
        
        contextMenu.addAction("编辑", this, [this, itemId]() {
            emit editTodoRequested(itemId);
        });
        contextMenu.addAction("删除", this, [this, itemId]() {
            emit deleteTodoRequested(itemId);
        });
        contextMenu.addSeparator();
    }
    
    contextMenu.addAction("刷新", this, &DesktopWidget::refreshDisplay);
    contextMenu.addSeparator();
    contextMenu.addAction("打开主窗口", this, [this]() { emit showMainWindowRequested(); });
    contextMenu.addAction("隐藏小贴士", this, &DesktopWidget::hide);
    
    contextMenu.exec(event->globalPos());
}

void DesktopWidget::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void DesktopWidget::saveWindowPosition()
{
    QSettings settings;
    settings.setValue("DesktopWidget/position", pos());
}

void DesktopWidget::loadWindowPosition()
{
    QSettings settings;
    QPoint savedPos = settings.value("DesktopWidget/position", QPoint(100, 100)).toPoint();
    
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    
    if (savedPos.x() < 0 || savedPos.x() > screenGeometry.width() - width()) {
        savedPos.setX(screenGeometry.width() - width() - 50);
    }
    if (savedPos.y() < 0 || savedPos.y() > screenGeometry.height() - height()) {
        savedPos.setY(100);
    }
    
    move(savedPos);
}
