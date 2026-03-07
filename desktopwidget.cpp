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
    , m_resizing(false)
    , m_resizeEdge(0)
{
    m_folders.clear();
    
    setupUI();
    
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(60000);
    
    setupConnections();
    
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(250, 300);
    
    loadWindowSize();
    loadWindowPosition();
    
    m_refreshTimer->start();
    
    applyStyles();
    loadPendingItems();
    
    setMouseTracking(true);
    m_todoListWidget->setMouseTracking(true);
    m_todoListWidget->installEventFilter(this);
}

DesktopWidget::~DesktopWidget()
{
    saveWindowPosition();
    saveWindowSize();
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
        "QListWidget::item { padding: 10px 12px; border-radius: 6px; margin: 2px 0; border-bottom: 1px solid #f1f5f9; }"
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
        listItem->setText(item.getTitle());
        listItem->setData(Qt::UserRole, item.getId());
        listItem->setForeground(QColor(51, 65, 85));
        
        m_todoListWidget->addItem(listItem);
    }
    
    int pendingCount = m_displayItems.size();
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
        QPoint pos = event->pos();
        
        if (isOnResizeArea(pos)) {
            m_resizing = true;
            m_resizeStartPos = event->globalPosition().toPoint();
            m_resizeStartSize = size();
            m_resizeStartWindowPos = this->pos();
            event->accept();
        } else {
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            m_dragging = true;
            event->accept();
        }
    }
}

void DesktopWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    
    if (m_resizing) {
        QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        
        int newWidth = m_resizeStartSize.width();
        int newHeight = m_resizeStartSize.height();
        int newX = m_resizeStartWindowPos.x();
        int newY = m_resizeStartWindowPos.y();
        
        if (m_resizeEdge & 0x01) {
            newWidth = qMax(minimumWidth(), m_resizeStartSize.width() + delta.x());
        }
        if (m_resizeEdge & 0x02) {
            newWidth = qMax(minimumWidth(), m_resizeStartSize.width() - delta.x());
            newX = m_resizeStartWindowPos.x() + (m_resizeStartSize.width() - newWidth);
        }
        if (m_resizeEdge & 0x04) {
            newHeight = qMax(minimumHeight(), m_resizeStartSize.height() + delta.y());
        }
        if (m_resizeEdge & 0x08) {
            newHeight = qMax(minimumHeight(), m_resizeStartSize.height() - delta.y());
            newY = m_resizeStartWindowPos.y() + (m_resizeStartSize.height() - newHeight);
        }
        
        resize(newWidth, newHeight);
        move(newX, newY);
        event->accept();
    } else if (event->buttons() & Qt::LeftButton && m_dragging) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    } else {
        updateCursor(pos);
    }
}

void DesktopWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_resizing = false;
        m_resizeEdge = 0;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
}

bool DesktopWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_todoListWidget && event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalPos = mouseEvent->globalPosition().toPoint();
        QPoint localPos = mapFromGlobal(globalPos);
        updateCursor(localPos);
    }
    return QWidget::eventFilter(obj, event);
}

void DesktopWidget::updateCursor(const QPoint &pos)
{
    if (m_dragging || m_resizing) return;
    
    int edge = 0;
    int margin = 8;
    
    if (pos.x() >= width() - margin) edge |= 0x01;
    if (pos.x() <= margin) edge |= 0x02;
    if (pos.y() >= height() - margin) edge |= 0x04;
    if (pos.y() <= margin) edge |= 0x08;
    
    Qt::CursorShape cursor = Qt::ArrowCursor;
    if (edge == 0x01 || edge == 0x02) cursor = Qt::SizeHorCursor;
    else if (edge == 0x04 || edge == 0x08) cursor = Qt::SizeVerCursor;
    else if (edge == 0x05 || edge == 0x0A) cursor = Qt::SizeFDiagCursor;
    else if (edge == 0x06 || edge == 0x09) cursor = Qt::SizeBDiagCursor;
    
    setCursor(cursor);
}

bool DesktopWidget::isOnResizeArea(const QPoint &pos)
{
    int margin = 8;
    m_resizeEdge = 0;
    
    if (pos.x() >= width() - margin) m_resizeEdge |= 0x01;
    if (pos.x() <= margin) m_resizeEdge |= 0x02;
    if (pos.y() >= height() - margin) m_resizeEdge |= 0x04;
    if (pos.y() <= margin) m_resizeEdge |= 0x08;
    
    return m_resizeEdge != 0;
}

QRect DesktopWidget::getResizeRect()
{
    return QRect(width() - 16, height() - 16, 16, 16);
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

void DesktopWidget::saveWindowSize()
{
    QSettings settings;
    settings.setValue("DesktopWidget/size", size());
}

void DesktopWidget::loadWindowSize()
{
    QSettings settings;
    QSize savedSize = settings.value("DesktopWidget/size", QSize(300, 420)).toSize();
    
    savedSize = savedSize.expandedTo(minimumSize());
    
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    if (savedSize.width() > screenGeometry.width()) {
        savedSize.setWidth(screenGeometry.width() - 50);
    }
    if (savedSize.height() > screenGeometry.height()) {
        savedSize.setHeight(screenGeometry.height() - 50);
    }
    
    resize(savedSize);
}
