#include "desktopwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QCloseEvent>
#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QFont>
#include <QFontMetrics>

DesktopWidget::DesktopWidget(QWidget *parent)
    : QWidget(parent)
    , m_trayIcon(nullptr)
    , m_dragging(false)
{
    setupUI();
    setupTrayIcon();
    setupConnections();
    
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(280, 400);
    
    // 加载窗口位置
    loadWindowPosition();
    
    // 启动刷新定时器
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(30000); // 30秒刷新一次
    m_refreshTimer->start();
    
    applyStyles();
}

DesktopWidget::~DesktopWidget()
{
    saveWindowPosition();
}

void DesktopWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(8);
    
    // 标题栏
    m_titleLabel = new QLabel("待办事项");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #333;");
    m_mainLayout->addWidget(m_titleLabel);
    
    // 待办事项列表
    m_todoListWidget = new QListWidget();
    m_todoListWidget->setMaximumHeight(250);
    m_mainLayout->addWidget(m_todoListWidget);
    
    // 添加新待办事项的布局
    m_addLayout = new QHBoxLayout();
    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("快速添加待办事项...");
    m_addButton = new QPushButton("添加");
    m_addButton->setMaximumWidth(50);
    
    m_addLayout->addWidget(m_addLineEdit);
    m_addLayout->addWidget(m_addButton);
    m_mainLayout->addLayout(m_addLayout);
    
    // 底部按钮布局
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton("刷新");
    m_settingsButton = new QPushButton("设置");
    
    bottomLayout->addWidget(m_refreshButton);
    bottomLayout->addWidget(m_settingsButton);
    m_mainLayout->addLayout(bottomLayout);
}

void DesktopWidget::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/todo.png")); // 需要添加图标资源
    m_trayIcon->setToolTip("Todo List - 待办事项");
    
    // 创建托盘菜单
    m_trayMenu = new QMenu(this);
    m_showAction = m_trayMenu->addAction("显示小贴士");
    m_trayMenu->addSeparator();
    QAction *showMainAction = m_trayMenu->addAction("打开主窗口");
    m_exitAction = m_trayMenu->addAction("退出");
    
    m_trayIcon->setContextMenu(m_trayMenu);
    
    // 连接信号
    connect(m_showAction, &QAction::triggered, this, &DesktopWidget::show);
    connect(showMainAction, &QAction::triggered, this, &DesktopWidget::onShowMainWindow);
    connect(m_exitAction, &QAction::triggered, this, &DesktopWidget::onExitApplication);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &DesktopWidget::onTrayIconActivated);
    
    m_trayIcon->show();
}

void DesktopWidget::setupConnections()
{
    connect(m_addButton, &QPushButton::clicked, this, &DesktopWidget::onAddTodoClicked);
    connect(m_addLineEdit, &QLineEdit::returnPressed, this, &DesktopWidget::onAddTodoClicked);
    connect(m_todoListWidget, &QListWidget::itemClicked, this, &DesktopWidget::onTodoItemClicked);
    connect(m_todoListWidget, &QListWidget::itemChanged, this, &DesktopWidget::onTodoItemChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, &DesktopWidget::refreshDisplay);
    connect(m_settingsButton, &QPushButton::clicked, this, &DesktopWidget::onShowMainWindow);
    connect(m_refreshTimer, &QTimer::timeout, this, &DesktopWidget::onRefreshTimer);
}

void DesktopWidget::applyStyles()
{
    setStyleSheet(
        "DesktopWidget {"
        "    background-color: rgba(255, 255, 255, 240);"
        "    border: 2px solid #ddd;"
        "    border-radius: 10px;"
        "}"
        "QListWidget {"
        "    background-color: rgba(255, 255, 255, 200);"
        "    border: 1px solid #ccc;"
        "    border-radius: 5px;"
        "    font-size: 12px;"
        "}"
        "QListWidget::item {"
        "    padding: 5px;"
        "    border-bottom: 1px solid #eee;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: rgba(230, 247, 255, 150);"
        "}"
        "QListWidget::item:selected {"
        "    background-color: rgba(100, 181, 246, 100);"
        "}"
        "QLineEdit {"
        "    padding: 5px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    font-size: 12px;"
        "}"
        "QPushButton {"
        "    padding: 5px 10px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    background-color: rgba(245, 245, 245, 200);"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(230, 230, 230, 200);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(200, 200, 200, 200);"
        "}"
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
    
    // 收集所有未完成的待办事项，最多显示10个
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
    
    // 更新标题显示待办事项数量
    int pendingCount = 0;
    for (const TodoItem &item : m_displayItems) {
        if (!item.isCompleted()) pendingCount++;
    }
    m_titleLabel->setText(QString("待办事项 (%1)").arg(pendingCount));
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
    
    QString itemId = item->data(Qt::UserRole).toString();
    bool completed = (item->checkState() == Qt::Checked);
    
    emit todoItemToggled(itemId, completed);
}

void DesktopWidget::onTodoItemChanged(QListWidgetItem *item)
{
    if (!item) return;
    
    QString itemId = item->data(Qt::UserRole).toString();
    bool completed = (item->checkState() == Qt::Checked);
    
    emit todoItemToggled(itemId, completed);
}

void DesktopWidget::onRefreshTimer()
{
    refreshDisplay();
}

void DesktopWidget::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if (isVisible()) {
            hide();
        } else {
            show();
            raise();
            activateWindow();
        }
        break;
    default:
        break;
    }
}

void DesktopWidget::onShowMainWindow()
{
    emit showMainWindowRequested();
}

void DesktopWidget::onExitApplication()
{
    QApplication::quit();
}

void DesktopWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制半透明背景
    painter.setBrush(QBrush(QColor(255, 255, 255, 240)));
    painter.setPen(QPen(QColor(200, 200, 200), 2));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);
    
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
    contextMenu.addAction("刷新", this, &DesktopWidget::refreshDisplay);
    contextMenu.addSeparator();
    contextMenu.addAction("打开主窗口", this, &DesktopWidget::onShowMainWindow);
    contextMenu.addAction("隐藏小贴士", this, &DesktopWidget::hide);
    contextMenu.addSeparator();
    contextMenu.addAction("退出", this, &DesktopWidget::onExitApplication);
    
    contextMenu.exec(event->globalPos());
}

void DesktopWidget::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore(); // 不真正关闭，只是隐藏
}

void DesktopWidget::saveWindowPosition()
{
    QSettings settings;
    settings.setValue("DesktopWidget/geometry", saveGeometry());
    settings.setValue("DesktopWidget/position", pos());
}

void DesktopWidget::loadWindowPosition()
{
    QSettings settings;
    restoreGeometry(settings.value("DesktopWidget/geometry").toByteArray());
    
    QPoint savedPos = settings.value("DesktopWidget/position", QPoint(100, 100)).toPoint();
    
    // 确保窗口在屏幕范围内
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