#include "mainwindow.h"
#include "theme.h"
#include "icons.h"
#include "components/navbar.h"
#include "components/sectionheader.h"
#include "components/titlebar.h"
#include "components/aurorabackground.h"
#include "components/messageutils.h"
#include "../core/databasemanager.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#endif

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QCursor>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QTimer>
#include <QApplication>
#include <QStyle>
#include <QDesktopServices>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDate>
#include <algorithm>

namespace {

// ---- 数据角色约定 ----
constexpr int RoleId        = Qt::UserRole;
constexpr int RoleColor     = Qt::UserRole + 1;
constexpr int RoleCompleted = Qt::UserRole + 2;
constexpr int RolePriority  = Qt::UserRole + 3;
constexpr int RolePinned    = Qt::UserRole + 4;
constexpr int RoleTitle     = Qt::UserRole + 5;
constexpr int RoleSubText   = Qt::UserRole + 6;
constexpr int RoleDate      = Qt::UserRole + 7;
constexpr int RoleDoneCount = Qt::UserRole + 8;
constexpr int RoleTotalCount= Qt::UserRole + 9;
constexpr int RoleFolderId  = Qt::UserRole + 20;   // 搜索结果项记录所属文件夹

QPixmap coloredDot(const QColor &color, int size = 12)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawEllipse(1, 1, size - 2, size - 2);
    return pm;
}

// 悬停淡入动画基类：跟踪当前悬停项，背景色平滑过渡
class AnimatedDelegate : public QStyledItemDelegate
{
public:
    explicit AnimatedDelegate(QListWidget *view) : QStyledItemDelegate(view), m_view(view) {}

protected:
    qreal hoverProgress(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QPersistentModelIndex persistent(index);
        bool hovered = option.state & QStyle::State_MouseOver;

        if (hovered && m_hoverIndex != persistent) {
            m_hoverIndex = persistent;
            startHoverAnimation();
        } else if (!hovered && m_hoverIndex == persistent) {
            m_hoverIndex = QPersistentModelIndex();
        }
        return (m_hoverIndex == persistent) ? m_hoverProgress : 0.0;
    }

    void startHoverAnimation() const
    {
        auto *anim = new QVariantAnimation;
        anim->setDuration(180);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        QObject::connect(anim, &QVariantAnimation::valueChanged,
                         m_view->viewport(), [this](const QVariant &v) {
            m_hoverProgress = v.toReal();
            m_view->viewport()->update();
        });
        QObject::connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
        anim->start();
    }

    QListWidget *m_view;
    mutable QPersistentModelIndex m_hoverIndex;
    mutable qreal m_hoverProgress = 0.0;
};

// 文件夹列表代理：色点 + 名称 + 置顶图标 + 完成统计徽标
class FolderDelegate : public AnimatedDelegate
{
public:
    using AnimatedDelegate::AnimatedDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        const bool selected = option.state & QStyle::State_Selected;
        const qreal hover = hoverProgress(option, index);
        const QColor folderColor(index.data(RoleColor).toString());
        const bool pinned = index.data(RolePinned).toBool();
        const QString name = index.data(RoleTitle).toString();
        const int done = index.data(RoleDoneCount).toInt();
        const int total = index.data(RoleTotalCount).toInt();

        QRect card = option.rect.adjusted(6, 3, -6, -3);

        // 背景：玻璃质感，选中时透主题色光晕
        QColor bg = Theme::glassBg();
        if (selected) {
            bg = Theme::withAlpha(Theme::primary(), Theme::isDark() ? 38 : 30);
        } else if (hover > 0.0) {
            const QColor h = Theme::glassBgStrong();
            const QColor base = Theme::glassBg();
            bg = Theme::mix(base, h, hover);
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(bg);
        painter->drawRoundedRect(card, Theme::radiusMd, Theme::radiusMd);

        // 玻璃高光描边
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(Theme::withAlpha(Theme::glassHighlight(), selected ? 70 : 26), 1));
        painter->drawRoundedRect(card.adjusted(0, 0, -1, -1), Theme::radiusMd, Theme::radiusMd);

        // 选中左侧霓虹条（带发光）
        if (selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::withAlpha(Theme::primary(), 60));
            painter->drawRoundedRect(card.left() - 1, card.top() + 6, 7, card.height() - 12, 3, 3);
            painter->setBrush(Theme::primary());
            painter->drawRoundedRect(card.left(), card.top() + 8, 4, card.height() - 16, 2, 2);
        }

        int x = card.left() + 14;
        int cy = card.center().y();

        // 文件夹色点（外圈发光）
        painter->setBrush(Theme::withAlpha(folderColor, 50));
        painter->drawEllipse(x - 3, cy - 8, 16, 16);
        painter->setBrush(folderColor);
        painter->drawEllipse(x, cy - 5, 10, 10);
        x += 20;

        // 置顶图标
        if (pinned) {
            QPixmap pin = Icons::pixmap(Icons::Pin, 13, Theme::warning());
            painter->drawPixmap(x, cy - 6, pin);
            x += 18;
        }

        // 统计徽标（先算宽度，右对齐）
        QString badge = QStringLiteral("%1/%2").arg(done).arg(total);
        QFont badgeFont = Theme::font(Theme::fontSmall);
        QFontMetrics bfm(badgeFont);
        int badgeW = bfm.horizontalAdvance(badge) + 18;
        QRect badgeRect(card.right() - badgeW - 10, cy - 10, badgeW, 20);
        painter->setBrush(selected ? Theme::withAlpha(Theme::primary(), 40)
                                   : Theme::glassBgStrong());
        painter->drawRoundedRect(badgeRect, 10, 10);
        painter->setFont(badgeFont);
        painter->setPen(selected ? Theme::primary() : Theme::textSecondary());
        painter->drawText(badgeRect, Qt::AlignCenter, badge);

        // 名称
        QFont nameFont = Theme::font(Theme::fontBase, selected ? QFont::DemiBold : QFont::Medium);
        painter->setFont(nameFont);
        painter->setPen(selected ? Theme::primaryHover() : Theme::textPrimary());
        QFontMetrics nfm(nameFont);
        int nameMax = badgeRect.left() - x - 10;
        painter->drawText(QRect(x, card.top(), qMax(nameMax, 10), card.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          nfm.elidedText(name, Qt::ElideRight, qMax(nameMax, 10)));

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(200, 48);
    }
};

// 待办列表代理：复选框 + 标签色条 + 优先级点 + 标题/日期/摘要 + 置顶
class TodoDelegate : public AnimatedDelegate
{
public:
    using AnimatedDelegate::AnimatedDelegate;

    static QRect checkRectFor(const QRect &itemRect)
    {
        QRect card = itemRect.adjusted(6, 4, -6, -4);
        return QRect(card.left() + 12, card.center().y() - 10, 20, 20);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        const bool selected = option.state & QStyle::State_Selected;
        const qreal hover = hoverProgress(option, index);
        const bool completed = index.data(RoleCompleted).toBool();
        const int priority = index.data(RolePriority).toInt();
        const bool pinned = index.data(RolePinned).toBool();
        const QString tagColor = index.data(RoleColor).toString();
        const QString title = index.data(RoleTitle).toString();
        const QString sub = index.data(RoleSubText).toString();
        const QString date = index.data(RoleDate).toString();

        QRect card = option.rect.adjusted(6, 4, -6, -4);

        // 卡片背景：玻璃质感，悬停时透出霓虹光晕
        QColor bg = Theme::glassBg();
        if (selected) {
            bg = Theme::withAlpha(Theme::primary(), Theme::isDark() ? 34 : 26);
        } else if (hover > 0.0) {
            const QColor h = Theme::hoverGlow();
            const QColor base = Theme::glassBg();
            bg = Theme::mix(base, h, hover);
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(bg);
        painter->drawRoundedRect(card, Theme::radiusMd, Theme::radiusMd);

        // 玻璃高光描边（悬停/选中更亮）
        painter->setBrush(Qt::NoBrush);
        const int borderAlpha = selected ? 80 : (hover > 0.0 ? static_cast<int>(26 + 44 * hover) : 22);
        painter->setPen(QPen(Theme::withAlpha(selected ? Theme::primary() : Theme::glassHighlight(), borderAlpha), 1));
        painter->drawRoundedRect(card.adjusted(0, 0, -1, -1), Theme::radiusMd, Theme::radiusMd);

        // 标签色条（卡片左缘，带发光）
        if (!tagColor.isEmpty()) {
            QColor c(tagColor);
            if (completed) c = Theme::withAlpha(c, 110);
            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::withAlpha(c, 45));
            painter->drawRoundedRect(card.left() + 1, card.top() + 6, 7, card.height() - 12, 3, 3);
            painter->setBrush(c);
            painter->drawRoundedRect(card.left() + 2, card.top() + 8, 4, card.height() - 16, 2, 2);
        }

        // 复选框
        QRect checkRect = checkRectFor(option.rect);
        if (completed) {
            QLinearGradient grad(checkRect.topLeft(), checkRect.bottomRight());
            grad.setColorAt(0, Theme::primary());
            grad.setColorAt(1, Theme::accent());
            painter->setBrush(grad);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(checkRect, 6, 6);
            QPainterPath checkPath;
            checkPath.moveTo(checkRect.left() + 5, checkRect.center().y());
            checkPath.lineTo(checkRect.center().x(), checkRect.bottom() - 5);
            checkPath.lineTo(checkRect.right() - 5, checkRect.top() + 5);
            painter->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawPath(checkPath);
        } else {
            painter->setPen(QPen(hover > 0.0 ? Theme::primary() : Theme::borderStrong(), 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(checkRect, 6, 6);
        }

        int x = checkRect.right() + 12;
        int titleY = card.top() + 10;

        // 日期（右上）
        QFont dateFont = Theme::font(Theme::fontSmall);
        QFontMetrics dfm(dateFont);
        int dateW = date.isEmpty() ? 0 : dfm.horizontalAdvance(date) + 6;
        QRect dateRect(card.right() - dateW - 12, titleY, dateW, 20);
        if (!date.isEmpty()) {
            painter->setFont(dateFont);
            painter->setPen(Theme::textMuted());
            painter->drawText(dateRect, Qt::AlignRight | Qt::AlignVCenter, date);
        }

        // 置顶图标
        int rightBoundary = dateRect.left() - 6;
        if (pinned) {
            QPixmap pin = Icons::pixmap(Icons::Pin, 13, Theme::warning());
            QRect pinRect(rightBoundary - 16, titleY + 3, 13, 13);
            painter->drawPixmap(pinRect, pin);
            rightBoundary = pinRect.left() - 4;
        }

        // 优先级点（发光）
        if (priority > 0 && !completed) {
            QColor pc = (priority == 2) ? Theme::danger() : Theme::warning();
            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::withAlpha(pc, 55));
            painter->drawEllipse(x - 3, titleY + 4, 13, 13);
            painter->setBrush(pc);
            painter->drawEllipse(x, titleY + 7, 7, 7);
            x += 14;
        }

        // 标题
        QFont titleFont = Theme::font(Theme::fontMedium, completed ? QFont::Normal : QFont::DemiBold);
        titleFont.setStrikeOut(completed);
        painter->setFont(titleFont);
        painter->setPen(completed ? Theme::textDisabled() : Theme::textPrimary());
        QFontMetrics tfm(titleFont);
        int titleMax = qMax(rightBoundary - x - 4, 20);
        painter->drawText(QRect(x, titleY, titleMax, 20), Qt::AlignLeft | Qt::AlignVCenter,
                          tfm.elidedText(title, Qt::ElideRight, titleMax));

        // 摘要行
        if (!sub.isEmpty()) {
            QFont subFont = Theme::font(Theme::fontSmall);
            painter->setFont(subFont);
            painter->setPen(Theme::textMuted());
            QFontMetrics sfm(subFont);
            QRect subRect(x, card.bottom() - 26, card.right() - x - 14, 18);
            painter->drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter,
                              sfm.elidedText(sub, Qt::ElideRight, subRect.width()));
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(240, 64);
    }
};

} // namespace

// ==========================================================
// 构造 / 析构
// ==========================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Todo List - 待办事项管理"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app.ico")));
    resize(1280, 760);
    setMinimumSize(960, 600);

    // 无边框窗口：原生标题栏由自定义 TitleBar 替代
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

#ifdef Q_OS_WIN
    // 为无边框窗口补回 DWM 阴影
    HWND hwnd = reinterpret_cast<HWND>(winId());
    MARGINS shadowMargins = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &shadowMargins);
#endif

    auto &db = DatabaseManager::instance();
    if (!db.initialize()) {
        MessageUtils::showError(this, QStringLiteral("数据库错误"), db.lastError());
    }

    m_folders = db.loadAll();
    if (m_folders.isEmpty()) {
        // 首次使用：创建默认数据
        TodoFolder defaultFolder(QStringLiteral("默认文件夹"));
        TodoItem sample(QStringLiteral("欢迎使用 Todo List"),
                        QStringLiteral("这是一个示例待办事项，可以编辑或删除它。"));
        defaultFolder.addItem(sample);
        m_folders.append(defaultFolder);
        db.upsertFolder(defaultFolder);
        db.upsertItem(sample);
    }

    buildUi();
    setupSystemTray();
    setupDesktopWidget();
    setupCalendarWidget();
    setupTagWidget();

    m_statsWidget = new StatsWidget(this);
    m_statsWidget->setData(m_folders);

    m_stack->addWidget(buildListPage());
    m_stack->addWidget(m_calendarWidget);
    m_stack->addWidget(m_tagWidget);
    m_stack->addWidget(m_statsWidget);
    m_navBar->attachStack(m_stack);

    // 到期提醒扫描（每分钟）
    m_reminderTimer = new QTimer(this);
    m_reminderTimer->setInterval(60000);
    connect(m_reminderTimer, &QTimer::timeout, this, &MainWindow::checkReminders);
    m_reminderTimer->start();
    QTimer::singleShot(3000, this, &MainWindow::checkReminders);   // 启动后先扫一次

    // 必须在所有页面控件创建完成之后再连接信号
    // （此前在 buildListPage() 之前调用，所有连接目标均为 nullptr，导致功能全部失效）
    setupConnections();

    updateFolderList();
    updateCalendarWidget();
    updateTagWidget();
    updateStatusBar();

    if (!m_folders.isEmpty()) {
        m_folderList->setCurrentRow(0);
    }

    if (m_desktopWidget) {
        m_desktopWidget->show();
    }
}

MainWindow::~MainWindow()
{
    saveSplitterState();
    if (m_desktopWidget) {
        m_desktopWidget->close();
        delete m_desktopWidget;
    }
}

// ==========================================================
// UI 构建
// ==========================================================

void MainWindow::buildUi()
{
    // 极光背景作为中央容器：标题栏/导航栏/页面全部半透明，透出流动光斑
    auto *central = new AuroraBackground(this);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_titleBar = new TitleBar(QStringLiteral("Todo List"), this);
    root->addWidget(m_titleBar);

    m_navBar = new NavBar(QString(),      // 应用名由标题栏展示，导航栏不再重复
                          {QStringLiteral("列表"), QStringLiteral("日历"),
                           QStringLiteral("标签"), QStringLiteral("统计")}, this);

    // 全局搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索待办…"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedWidth(190);
    m_navBar->addRightWidget(m_searchEdit);

    // 右侧操作按钮（悬停霓虹光晕动效）
    auto *desktopBtn = new NavIconButton(Icons::Desktop, QStringLiteral("桌面小贴士"), this);
    connect(desktopBtn, &QPushButton::clicked, this, &MainWindow::onDesktopWidgetClicked);
    m_navBar->addRightWidget(desktopBtn);

    auto *backupBtn = new NavIconButton(Icons::Backup, QStringLiteral("立即备份"), this);
    connect(backupBtn, &QPushButton::clicked, this, &MainWindow::onBackupClicked);
    m_navBar->addRightWidget(backupBtn);

    auto *menuBtn = new NavIconButton(Icons::List, QStringLiteral("菜单"), this);
    m_navBar->addRightWidget(menuBtn);

    QMenu *appMenu = new QMenu(this);
    QAction *darkAction = appMenu->addAction(QStringLiteral("深色模式"), this, &MainWindow::onToggleDarkMode);
    darkAction->setCheckable(true);
    darkAction->setChecked(Theme::isDark());
    appMenu->addSeparator();
    appMenu->addAction(Icons::icon(Icons::Import, 14, Theme::textSecondary()),
                       QStringLiteral("导入数据"), this, &MainWindow::onImportClicked);
    appMenu->addAction(Icons::icon(Icons::Export, 14, Theme::textSecondary()),
                       QStringLiteral("导出数据"), this, &MainWindow::onExportClicked);
    appMenu->addSeparator();
    appMenu->addAction(Icons::icon(Icons::Folder, 14, Theme::textSecondary()),
                       QStringLiteral("打开备份目录"), this, &MainWindow::onOpenBackupDir);
    appMenu->addSeparator();
    appMenu->addAction(Icons::icon(Icons::Exit, 14, Theme::textSecondary()),
                       QStringLiteral("退出"), this, &MainWindow::onExitClicked);
    connect(menuBtn, &QPushButton::clicked, this, [this, menuBtn, appMenu]() {
        appMenu->exec(menuBtn->mapToGlobal(QPoint(0, menuBtn->height())));
    });

    root->addWidget(m_navBar);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack, 1);

    setCentralWidget(central);

    // 状态栏
    m_statusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusLabel);
}

QWidget* MainWindow::buildListPage()
{
    QWidget *page = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(page);
    layout->setContentsMargins(20, 18, 20, 20);
    layout->setSpacing(14);

    m_mainSplitter = new QSplitter(Qt::Horizontal, page);
    m_mainSplitter->setHandleWidth(2);
    m_mainSplitter->setChildrenCollapsible(false);

    m_mainSplitter->addWidget(buildFolderPanel());
    m_mainSplitter->addWidget(buildTodoPanel());
    m_mainSplitter->addWidget(buildDetailPanel());

    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 0);

    layout->addWidget(m_mainSplitter);

    setupSplitter();
    loadSplitterState();
    return page;
}

QWidget* MainWindow::buildFolderPanel()
{
    QWidget *panel = new QWidget(this);
    panel->setMinimumWidth(230);
    panel->setMaximumWidth(300);
    panel->setObjectName(QStringLiteral("folderPanel"));
    panel->setStyleSheet(Theme::glassPanelStyle(panel->objectName()));

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto *header = new SectionHeader(QStringLiteral("文件夹"), panel);
    layout->addWidget(header);

    m_newFolderBtn = new QPushButton(QStringLiteral("新建文件夹"), panel);
    m_newFolderBtn->setProperty("variant", "success");
    m_newFolderBtn->setIcon(Icons::icon(Icons::Plus, 14, Theme::success()));
    m_newFolderBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_newFolderBtn);

    m_folderList = new QListWidget(panel);
    m_folderList->setItemDelegate(new FolderDelegate(m_folderList));
    m_folderList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_folderList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderList->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_folderList->setAcceptDrops(false);
    m_folderList->viewport()->setAcceptDrops(true);
    m_folderList->viewport()->installEventFilter(this);
    layout->addWidget(m_folderList, 1);

    m_folderEmptyHint = new QLabel(QStringLiteral("还没有文件夹\n点击上方按钮创建一个吧"), panel);
    m_folderEmptyHint->setAlignment(Qt::AlignCenter);
    m_folderEmptyHint->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; padding: 20px;")
                                     .arg(Theme::textMuted().name()));
    m_folderEmptyHint->setVisible(false);
    layout->addWidget(m_folderEmptyHint);

    // 回收站入口（底部）
    auto *trashBtn = new QPushButton(QStringLiteral("回收站"), panel);
    trashBtn->setProperty("variant", "ghost");
    trashBtn->setIcon(Icons::icon(Icons::Trash, 14, Theme::textSecondary()));
    trashBtn->setCursor(Qt::PointingHandCursor);
    trashBtn->setToolTip(QStringLiteral("查看已删除的待办（保留 30 天）"));
    connect(trashBtn, &QPushButton::clicked, this, &MainWindow::onTrashClicked);
    layout->addWidget(trashBtn);

    return panel;
}

QWidget* MainWindow::buildTodoPanel()
{
    QWidget *panel = new QWidget(this);
    panel->setMinimumWidth(380);
    panel->setObjectName(QStringLiteral("todoPanel"));
    panel->setStyleSheet(Theme::glassPanelStyle(panel->objectName()));

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    m_todoHeader = new SectionHeader(QStringLiteral("待办事项"), panel);
    layout->addWidget(m_todoHeader);

    // 快速添加行
    QHBoxLayout *addRow = new QHBoxLayout();
    addRow->setSpacing(8);
    m_quickAddEdit = new QLineEdit(panel);
    m_quickAddEdit->setPlaceholderText(QStringLiteral("输入标题，回车快速创建待办"));
    m_quickAddEdit->setClearButtonEnabled(true);
    addRow->addWidget(m_quickAddEdit, 1);

    m_newTodoBtn = new QPushButton(QStringLiteral("新建"), panel);
    m_newTodoBtn->setProperty("variant", "primary");
    m_newTodoBtn->setIcon(Icons::icon(Icons::Plus, 14, Qt::white));
    m_newTodoBtn->setCursor(Qt::PointingHandCursor);
    m_newTodoBtn->setToolTip(QStringLiteral("创建带详情的待办事项"));
    addRow->addWidget(m_newTodoBtn);
    layout->addLayout(addRow);

    m_todoList = new QListWidget(panel);
    m_todoList->setItemDelegate(new TodoDelegate(m_todoList));
    m_todoList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_todoList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_todoList->setDragEnabled(true);
    m_todoList->setDragDropMode(QAbstractItemView::DragOnly);
    layout->addWidget(m_todoList, 1);

    m_todoEmptyHint = new QLabel(QStringLiteral("选择一个文件夹查看待办事项"), panel);
    m_todoEmptyHint->setAlignment(Qt::AlignCenter);
    m_todoEmptyHint->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; padding: 20px;")
                                   .arg(Theme::textMuted().name()));
    m_todoEmptyHint->setVisible(false);
    layout->addWidget(m_todoEmptyHint);

    return panel;
}

QWidget* MainWindow::buildDetailPanel()
{
    QWidget *panel = new QWidget(this);
    panel->setMinimumWidth(320);
    panel->setMaximumWidth(400);
    panel->setObjectName(QStringLiteral("detailPanel"));
    panel->setStyleSheet(Theme::glassPanelStyle(panel->objectName()));

    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(16, 16, 16, 16);
    panelLayout->setSpacing(10);

    auto *header = new SectionHeader(QStringLiteral("详情"), panel);
    panelLayout->addWidget(header);

    m_detailScroll = new QScrollArea(panel);
    m_detailScroll->setWidgetResizable(true);
    m_detailScroll->setFrameShape(QFrame::NoFrame);

    m_detailCard = new QWidget();
    QVBoxLayout *card = new QVBoxLayout(m_detailCard);
    card->setContentsMargins(4, 4, 4, 4);
    card->setSpacing(10);

    const QString labelStyle = QStringLiteral("color: %1; font-size: 12px; font-weight: 500;")
                               .arg(Theme::textSecondary().name());
    auto makeLabel = [&labelStyle](const QString &text, QWidget *parent) {
        auto *l = new QLabel(text, parent);
        l->setStyleSheet(labelStyle);
        l->setProperty("sectionLabel", true);   // 供主题切换时统一刷新
        return l;
    };

    m_emptyStateLabel = new QLabel(QStringLiteral("请选择左侧的一个待办事项\n在这里查看和编辑详情"), m_detailCard);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setWordWrap(true);
    m_emptyStateLabel->setStyleSheet(
        QStringLiteral("color: %1; font-size: 13px; padding: 28px 16px; background-color: %2; border: 1px dashed %4; border-radius: %3px;")
        .arg(Theme::textMuted().name(), Theme::glassBg().name(QColor::HexArgb))
        .arg(Theme::radiusMd).arg(Theme::glassBorder().name(QColor::HexArgb)));
    card->addWidget(m_emptyStateLabel);

    card->addWidget(makeLabel(QStringLiteral("标题"), m_detailCard));
    m_titleEdit = new QLineEdit(m_detailCard);
    m_titleEdit->setPlaceholderText(QStringLiteral("请输入标题"));
    card->addWidget(m_titleEdit);

    card->addWidget(makeLabel(QStringLiteral("详细描述"), m_detailCard));
    m_detailsEdit = new QTextEdit(m_detailCard);
    m_detailsEdit->setMinimumHeight(90);
    m_detailsEdit->setPlaceholderText(QStringLiteral("记录更多细节..."));
    card->addWidget(m_detailsEdit);

    card->addWidget(makeLabel(QStringLiteral("优先级"), m_detailCard));
    m_priorityCombo = new QComboBox(m_detailCard);
    m_priorityCombo->addItem(coloredDot(Theme::textMuted()), QStringLiteral("低优先级"));
    m_priorityCombo->addItem(coloredDot(Theme::warning()), QStringLiteral("中优先级"));
    m_priorityCombo->addItem(coloredDot(Theme::danger()), QStringLiteral("高优先级"));
    card->addWidget(m_priorityCombo);

    card->addWidget(makeLabel(QStringLiteral("标签颜色"), m_detailCard));
    m_tagColorCombo = new QComboBox(m_detailCard);
    {
        const QStringList names = {QStringLiteral("蓝色"), QStringLiteral("绿色"), QStringLiteral("红色"),
                                   QStringLiteral("橙色"), QStringLiteral("紫色"), QStringLiteral("青色")};
        const QStringList colors = Theme::palette();
        for (int i = 0; i < names.size(); ++i) {
            m_tagColorCombo->addItem(coloredDot(QColor(colors[i])), names[i]);
        }
    }
    card->addWidget(m_tagColorCombo);

    card->addWidget(makeLabel(QStringLiteral("标签"), m_detailCard));
    QHBoxLayout *tagsRow = new QHBoxLayout();
    tagsRow->setContentsMargins(0, 0, 0, 0);
    tagsRow->setSpacing(6);
    m_tagsDisplayLabel = new QLabel(QStringLiteral("无标签"), m_detailCard);
    m_tagsDisplayLabel->setWordWrap(true);
    m_tagsDisplayLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;")
                                      .arg(Theme::textMuted().name()));
    tagsRow->addWidget(m_tagsDisplayLabel, 1);
    m_addTagBtn = new QPushButton(m_detailCard);
    m_addTagBtn->setProperty("variant", "ghost");
    m_addTagBtn->setIcon(Icons::icon(Icons::Plus, 14, Theme::primary()));
    m_addTagBtn->setFixedSize(28, 28);
    m_addTagBtn->setToolTip(QStringLiteral("添加标签"));
    m_addTagBtn->setCursor(Qt::PointingHandCursor);
    tagsRow->addWidget(m_addTagBtn);
    card->addLayout(tagsRow);

    card->addWidget(makeLabel(QStringLiteral("创建时间"), m_detailCard));
    m_createdTimeLabel = new QLabel(QStringLiteral("-"), m_detailCard);
    m_createdTimeLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;")
                                      .arg(Theme::textPrimary().name()));
    card->addWidget(m_createdTimeLabel);

    m_completedTimeTitle = makeLabel(QStringLiteral("完成时间"), m_detailCard);
    card->addWidget(m_completedTimeTitle);
    m_completedTimeLabel = new QLabel(QStringLiteral("-"), m_detailCard);
    m_completedTimeLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;")
                                        .arg(Theme::textPrimary().name()));
    card->addWidget(m_completedTimeLabel);

    m_completedCheck = new QCheckBox(QStringLiteral("标记为已完成"), m_detailCard);
    card->addWidget(m_completedCheck);

    m_remindCheck = new QCheckBox(QStringLiteral("到期当天早上 9 点提醒我"), m_detailCard);
    m_remindCheck->setToolTip(QStringLiteral("仅对设置了到期日期的待办生效"));
    card->addWidget(m_remindCheck);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    m_saveBtn = new QPushButton(QStringLiteral("保存"), m_detailCard);
    m_saveBtn->setProperty("variant", "primary");
    m_saveBtn->setIcon(Icons::icon(Icons::Save, 14, Qt::white));
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(m_saveBtn, 1);

    m_deleteBtn = new QPushButton(QStringLiteral("删除"), m_detailCard);
    m_deleteBtn->setProperty("variant", "danger");
    m_deleteBtn->setIcon(Icons::icon(Icons::Trash, 14, Theme::danger()));
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(m_deleteBtn, 1);
    card->addLayout(btnRow);

    card->addStretch();

    m_detailScroll->setWidget(m_detailCard);
    panelLayout->addWidget(m_detailScroll, 1);

    clearDetailPanel();
    return panel;
}

void MainWindow::setupSplitter()
{
    // 尺寸在 loadSplitterState 中恢复；此处无需额外处理
}

void MainWindow::saveSplitterState()
{
    if (!m_mainSplitter) return;
    QSettings settings;
    settings.setValue(QStringLiteral("main/splitterState"), m_mainSplitter->saveState());
}

void MainWindow::loadSplitterState()
{
    if (!m_mainSplitter) return;
    QSettings settings;
    if (settings.contains(QStringLiteral("main/splitterState"))) {
        m_mainSplitter->restoreState(settings.value(QStringLiteral("main/splitterState")).toByteArray());
    } else {
        m_mainSplitter->setSizes({250, 560, 340});
    }
}

void MainWindow::setupConnections()
{
    // 文件夹
    connect(m_newFolderBtn, &QPushButton::clicked, this, &MainWindow::onNewFolderClicked);
    connect(m_folderList, &QListWidget::currentRowChanged, this, &MainWindow::onFolderSelectionChanged);
    connect(m_folderList, &QListWidget::itemDoubleClicked, this, &MainWindow::onFolderDoubleClicked);
    connect(m_folderList, &QListWidget::customContextMenuRequested, this, &MainWindow::onFolderContextMenu);

    // 待办
    connect(m_newTodoBtn, &QPushButton::clicked, this, &MainWindow::onNewTodoClicked);
    connect(m_quickAddEdit, &QLineEdit::returnPressed, this, &MainWindow::onQuickAddTodo);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_todoList, &QListWidget::currentRowChanged, this, &MainWindow::onTodoSelectionChanged);
    connect(m_todoList, &QListWidget::itemClicked, this, &MainWindow::onTodoClicked);
    connect(m_todoList, &QListWidget::itemDoubleClicked, this, &MainWindow::onTodoDoubleClicked);
    connect(m_todoList, &QListWidget::customContextMenuRequested, this, &MainWindow::onTodoContextMenu);
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    connect(m_completedCheck, &QCheckBox::toggled, this, &MainWindow::onCompletedToggled);
    connect(m_remindCheck, &QCheckBox::toggled, this, &MainWindow::onRemindToggled);
    connect(m_addTagBtn, &QPushButton::clicked, this, [this]() {
        TodoItem *item = currentItem();
        if (!item) return;

        QStringList existing = DatabaseManager::instance().allTagNames();
        existing.sort(Qt::CaseInsensitive);
        QString newTag = MessageUtils::getItem(this, QStringLiteral("添加标签"),
                                               QStringLiteral("选择或输入标签:"), existing, 0, true);
        newTag = newTag.trimmed();
        if (!newTag.isEmpty()) {
            onTodoTagAdded(item->getId(), newTag);
        }
    });

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::onAboutToQuit);
}

// ==========================================================
// 数据访问辅助
// ==========================================================

TodoFolder* MainWindow::findFolderById(const QString &folderId)
{
    for (TodoFolder &folder : m_folders) {
        if (folder.getId() == folderId) {
            return &folder;
        }
    }
    return nullptr;
}

TodoItem* MainWindow::findTodoItemById(const QString &itemId, QString *outFolderId)
{
    for (TodoFolder &folder : m_folders) {
        if (TodoItem *item = folder.findItem(itemId)) {
            if (outFolderId) *outFolderId = folder.getId();
            return item;
        }
    }
    return nullptr;
}

TodoFolder* MainWindow::currentFolder()
{
    return m_currentFolderId.isEmpty() ? nullptr : findFolderById(m_currentFolderId);
}

TodoItem* MainWindow::currentItem()
{
    TodoFolder *folder = currentFolder();
    return (folder && !m_currentItemId.isEmpty()) ? folder->findItem(m_currentItemId) : nullptr;
}

void MainWindow::persistFolder(TodoFolder *folder)
{
    if (!folder) return;
    if (!DatabaseManager::instance().upsertFolder(*folder)) {
        MessageUtils::showError(this, QStringLiteral("保存失败"), DatabaseManager::instance().lastError());
    }
}

void MainWindow::persistItem(TodoItem *item)
{
    if (!item) return;
    if (!DatabaseManager::instance().upsertItem(*item)) {
        MessageUtils::showError(this, QStringLiteral("保存失败"), DatabaseManager::instance().lastError());
    }
}

// ==========================================================
// 文件夹操作
// ==========================================================

void MainWindow::onNewFolderClicked()
{
    QString name = MessageUtils::getText(this, QStringLiteral("新建文件夹"),
                                         QStringLiteral("请输入文件夹名称:"), QStringLiteral("新建文件夹"));
    name = name.trimmed();
    if (name.isEmpty()) return;

    TodoFolder folder(name);
    m_folders.append(folder);
    persistFolder(&m_folders.last());

    m_currentFolderId = folder.getId();
    m_currentItemId.clear();

    updateFolderList();
    updateTodoList();
    clearDetailPanel();
    updateStatusBar();
}

void MainWindow::onFolderSelectionChanged()
{
    QListWidgetItem *item = m_folderList->currentItem();
    QString newId = item ? item->data(RoleId).toString() : QString();

    if (newId != m_currentFolderId) {
        m_currentFolderId = newId;
        m_currentItemId.clear();
    }

    updateTodoList();
    if (!currentItem()) {
        clearDetailPanel();
    }
}

void MainWindow::onFolderDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    TodoFolder *folder = findFolderById(item->data(RoleId).toString());
    if (!folder) return;

    QString newName = MessageUtils::getText(this, QStringLiteral("重命名文件夹"),
                                            QStringLiteral("请输入新的文件夹名称:"), folder->getName());
    newName = newName.trimmed();
    if (newName.isEmpty()) return;

    folder->setName(newName);
    persistFolder(folder);
    updateFolderList();
    if (folder->getId() == m_currentFolderId) {
        m_todoHeader->setTitle(newName);
    }
}

void MainWindow::onFolderContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_folderList->itemAt(pos);
    if (!item) return;

    QString folderId = item->data(RoleId).toString();
    TodoFolder *folder = findFolderById(folderId);
    if (!folder) return;

    m_folderList->setCurrentItem(item);

    QMenu menu(this);
    QAction *pinAction = menu.addAction(Icons::icon(Icons::Pin, 14, Theme::warning()),
                                        folder->isPinned() ? QStringLiteral("取消置顶") : QStringLiteral("置顶"));
    QAction *renameAction = menu.addAction(Icons::icon(Icons::Edit, 14, Theme::textSecondary()),
                                           QStringLiteral("重命名"));

    QMenu *colorMenu = menu.addMenu(QStringLiteral("设置颜色"));
    const QStringList colorNames = {QStringLiteral("蓝色"), QStringLiteral("绿色"), QStringLiteral("红色"),
                                    QStringLiteral("橙色"), QStringLiteral("紫色"), QStringLiteral("青色")};
    const QStringList colorValues = Theme::palette();
    for (int i = 0; i < colorNames.size(); ++i) {
        colorMenu->addAction(coloredDot(QColor(colorValues[i])), colorNames[i], this, [this, folderId, colorValues, i]() {
            if (TodoFolder *f = findFolderById(folderId)) {
                f->setColor(colorValues[i]);
                persistFolder(f);
                updateFolderList();
            }
        });
    }

    menu.addSeparator();
    QAction *deleteAction = menu.addAction(Icons::icon(Icons::Trash, 14, Theme::danger()),
                                           QStringLiteral("删除"));

    QAction *chosen = menu.exec(m_folderList->mapToGlobal(pos));
    if (chosen == pinAction) {
        onPinFolderClicked();
    } else if (chosen == renameAction) {
        onFolderDoubleClicked(item);
    } else if (chosen == deleteAction) {
        onDeleteFolderClicked();
    }
}

void MainWindow::onPinFolderClicked()
{
    TodoFolder *folder = currentFolder();
    if (!folder) return;

    folder->setPinned(!folder->isPinned());
    persistFolder(folder);
    updateFolderList();
}

void MainWindow::onDeleteFolderClicked()
{
    TodoFolder *folder = currentFolder();
    if (!folder) {
        MessageUtils::showInfo(this, QStringLiteral("提示"), QStringLiteral("请先选择一个文件夹。"));
        return;
    }

    QString folderId = folder->getId();
    if (!MessageUtils::showConfirm(this, QStringLiteral("确认删除"),
            QStringLiteral("确定要删除文件夹 \"%1\" 及其所有待办事项吗？\n（可从自动备份中恢复）").arg(folder->getName()))) {
        return;
    }

    if (!DatabaseManager::instance().deleteFolder(folderId)) {
        MessageUtils::showError(this, QStringLiteral("删除失败"), DatabaseManager::instance().lastError());
        return;
    }

    for (int i = 0; i < m_folders.size(); ++i) {
        if (m_folders[i].getId() == folderId) {
            m_folders.removeAt(i);
            break;
        }
    }

    m_currentFolderId.clear();
    m_currentItemId.clear();

    updateFolderList();
    updateTodoList();
    clearDetailPanel();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
    updateStatusBar();

    if (!m_folders.isEmpty()) {
        m_folderList->setCurrentRow(0);
    }
}

// ==========================================================
// 待办操作
// ==========================================================

void MainWindow::onNewTodoClicked()
{
    TodoFolder *folder = currentFolder();
    if (!folder) {
        MessageUtils::showInfo(this, QStringLiteral("提示"), QStringLiteral("请先选择一个文件夹。"));
        return;
    }

    QString title = MessageUtils::getText(this, QStringLiteral("新建待办事项"),
                                          QStringLiteral("请输入待办事项标题:"));
    title = title.trimmed();
    if (title.isEmpty()) return;

    TodoItem item(title);
    folder->addItem(item);
    persistItem(folder->findItem(item.getId()));

    m_currentItemId = item.getId();
    updateTodoList();
    updateFolderList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
    updateStatusBar();
}

void MainWindow::onQuickAddTodo()
{
    TodoFolder *folder = currentFolder();
    if (!folder) {
        MessageUtils::showInfo(this, QStringLiteral("提示"), QStringLiteral("请先选择一个文件夹。"));
        return;
    }

    QString title = m_quickAddEdit->text().trimmed();
    if (title.isEmpty()) return;

    TodoItem item(title);
    folder->addItem(item);
    persistItem(folder->findItem(item.getId()));

    m_quickAddEdit->clear();
    m_currentItemId = item.getId();
    updateTodoList();
    updateFolderList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
    updateStatusBar();
}

void MainWindow::onTodoSelectionChanged()
{
    QListWidgetItem *item = m_todoList->currentItem();
    m_currentItemId = item ? item->data(RoleId).toString() : QString();

    if (currentItem()) {
        updateDetailPanel();
    } else {
        clearDetailPanel();
    }
}

void MainWindow::onTodoClicked(QListWidgetItem *item)
{
    if (!item) return;

    // 点击复选框区域：直接切换完成状态
    QRect itemRect = m_todoList->visualItemRect(item);
    if (TodoDelegate::checkRectFor(itemRect).contains(m_todoList->mapFromGlobal(QCursor::pos()))) {
        QString itemId = item->data(RoleId).toString();
        bool completed = item->data(RoleCompleted).toBool();
        toggleTodoCompleted(itemId, !completed);
    }
}

void MainWindow::onTodoDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item)
    // 双击：确保详情面板获得焦点
    if (currentItem() && m_titleEdit) {
        m_titleEdit->setFocus();
        m_titleEdit->selectAll();
    }
}

void MainWindow::onTodoContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_todoList->itemAt(pos);
    if (!item) return;

    QString itemId = item->data(RoleId).toString();
    TodoItem *todo = findTodoItemById(itemId);
    if (!todo) return;

    m_todoList->setCurrentItem(item);

    QMenu menu(this);
    QAction *pinAction = menu.addAction(Icons::icon(Icons::Pin, 14, Theme::warning()),
                                        todo->isPinned() ? QStringLiteral("取消置顶") : QStringLiteral("置顶"));

    QMenu *tagMenu = menu.addMenu(QStringLiteral("添加标签"));
    QStringList existing = DatabaseManager::instance().allTagNames();
    existing.sort(Qt::CaseInsensitive);
    if (existing.isEmpty()) {
        QAction *none = tagMenu->addAction(QStringLiteral("(暂无标签)"));
        none->setEnabled(false);
    } else {
        for (const QString &tag : existing) {
            tagMenu->addAction(tag, this, [this, itemId, tag]() { onTodoTagAdded(itemId, tag); });
        }
    }

    menu.addSeparator();
    QAction *deleteAction = menu.addAction(Icons::icon(Icons::Trash, 14, Theme::danger()),
                                           QStringLiteral("删除"));

    QAction *chosen = menu.exec(m_todoList->mapToGlobal(pos));
    if (chosen == pinAction) {
        if (TodoItem *t = findTodoItemById(itemId)) {
            t->setPinned(!t->isPinned());
            persistItem(t);
            updateTodoList();
        }
    } else if (chosen == deleteAction) {
        deleteTodoItem(itemId);
    }
}

void MainWindow::onSaveClicked()
{
    TodoItem *item = currentItem();
    if (!item) return;

    item->setTitle(m_titleEdit->text().trimmed().isEmpty() ? item->getTitle() : m_titleEdit->text().trimmed());
    item->setDetails(m_detailsEdit->toPlainText());
    item->setPriority(qBound(0, m_priorityCombo->currentIndex(), 2));

    const QStringList colors = Theme::palette();
    int colorIndex = m_tagColorCombo->currentIndex();
    if (colorIndex >= 0 && colorIndex < colors.size()) {
        item->setTagColor(colors[colorIndex]);
    }

    persistItem(item);
    updateTodoList();
    updateFolderList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();

    MessageUtils::showSuccess(this, QStringLiteral("保存"), QStringLiteral("修改已保存。"));
}

void MainWindow::onDeleteClicked()
{
    TodoItem *item = currentItem();
    if (!item) {
        MessageUtils::showInfo(this, QStringLiteral("提示"), QStringLiteral("请先选择一个待办事项。"));
        return;
    }
    deleteTodoItem(item->getId());
}

void MainWindow::onCompletedToggled(bool completed)
{
    if (m_currentItemId.isEmpty()) return;
    toggleTodoCompleted(m_currentItemId, completed);
}

void MainWindow::onRemindToggled(bool remind)
{
    TodoItem *item = currentItem();
    if (!item) return;

    if (remind) {
        const QDate due = item->getDueDate();
        if (!due.isValid()) {
            MessageUtils::showInfo(this, QStringLiteral("提醒"),
                                   QStringLiteral("这个待办还没有设置到期日期，请先在日历中安排日期。"));
            m_remindCheck->blockSignals(true);
            m_remindCheck->setChecked(false);
            m_remindCheck->blockSignals(false);
            return;
        }
        item->setRemindAt(QDateTime(due, QTime(9, 0)));   // 到期当天 09:00
    } else {
        item->setRemindAt(QDateTime());
    }
    persistItem(item);
}

bool MainWindow::toggleTodoCompleted(const QString &itemId, bool completed)
{
    QString folderId;
    TodoItem *item = findTodoItemById(itemId, &folderId);
    if (!item) return false;

    item->setCompleted(completed);
    persistItem(item);

    updateFolderList();
    updateTodoList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();

    if (m_currentItemId == itemId && currentItem()) {
        updateDetailPanel();
    }
    updateStatusBar();
    return true;
}

bool MainWindow::deleteTodoItem(const QString &itemId)
{
    QString folderId;
    TodoItem *item = findTodoItemById(itemId, &folderId);
    if (!item) return false;

    if (!MessageUtils::showConfirm(this, QStringLiteral("确认删除"),
            QStringLiteral("确定要删除待办事项 \"%1\" 吗？").arg(item->getTitle()))) {
        return false;
    }

    if (!DatabaseManager::instance().deleteItem(itemId)) {
        MessageUtils::showError(this, QStringLiteral("删除失败"), DatabaseManager::instance().lastError());
        return false;
    }

    if (TodoFolder *folder = findFolderById(folderId)) {
        folder->removeItem(itemId);
    }
    if (m_currentItemId == itemId) {
        m_currentItemId.clear();
    }

    refreshAllViews();
    clearDetailPanel();
    updateStatusBar();
    return true;
}

// ==========================================================
// 标签
// ==========================================================

void MainWindow::onTodoTagAdded(const QString &todoId, const QString &tag)
{
    TodoItem *item = findTodoItemById(todoId);
    if (!item) return;

    item->addTag(tag);
    persistItem(item);
    updateTodoTags();
    updateTodoList();
    updateTagWidget();
}

void MainWindow::onTodoTagRemoved(const QString &todoId, const QString &tag)
{
    TodoItem *item = findTodoItemById(todoId);
    if (!item) return;

    item->removeTag(tag);
    persistItem(item);
    updateTodoTags();
    updateTodoList();
    updateTagWidget();
}

// ==========================================================
// 视图刷新
// ==========================================================

void MainWindow::updateFolderList()
{
    std::sort(m_folders.begin(), m_folders.end(), [](const TodoFolder &a, const TodoFolder &b) {
        if (a.isPinned() != b.isPinned()) return a.isPinned() > b.isPinned();
        return a.getCreatedTime() > b.getCreatedTime();
    });

    m_folderList->blockSignals(true);
    m_folderList->clear();

    int selectRow = -1;
    for (int i = 0; i < m_folders.size(); ++i) {
        const TodoFolder &folder = m_folders[i];

        auto *item = new QListWidgetItem();
        item->setData(RoleId, folder.getId());
        item->setData(RoleTitle, folder.getName());
        item->setData(RoleColor, folder.getColor());
        item->setData(RolePinned, folder.isPinned());
        item->setData(RoleDoneCount, folder.getCompletedCount());
        item->setData(RoleTotalCount, folder.getItemCount());
        m_folderList->addItem(item);

        if (folder.getId() == m_currentFolderId) {
            selectRow = i;
        }
    }
    m_folderList->blockSignals(false);

    if (selectRow >= 0) {
        m_folderList->setCurrentRow(selectRow);
    } else if (m_currentFolderId.isEmpty() && m_folderList->count() > 0) {
        m_folderList->setCurrentRow(0);
    }

    m_folderEmptyHint->setVisible(m_folders.isEmpty());
}

void MainWindow::updateTodoList()
{
    if (m_searching) return;   // 搜索模式下列表由 showSearchResults 管理

    TodoFolder *folder = currentFolder();

    m_todoList->blockSignals(true);
    m_todoList->clear();

    if (!folder) {
        m_todoList->blockSignals(false);
        m_todoHeader->setTitle(QStringLiteral("待办事项"));
        m_todoEmptyHint->setText(QStringLiteral("选择一个文件夹查看待办事项"));
        m_todoEmptyHint->setVisible(true);
        return;
    }

    m_todoHeader->setTitle(folder->getName());

    QList<TodoItem> items = folder->getItems();
    std::sort(items.begin(), items.end(), [](const TodoItem &a, const TodoItem &b) {
        if (a.isPinned() != b.isPinned()) return a.isPinned() > b.isPinned();
        if (a.isCompleted() != b.isCompleted()) return a.isCompleted() < b.isCompleted();
        return a.getCreatedTime() > b.getCreatedTime();
    });

    int selectRow = -1;
    for (int i = 0; i < items.size(); ++i) {
        const TodoItem &todo = items[i];

        QString sub = todo.getDetails().split('\n').first().left(40);
        if (todo.getDetails().length() > 40) sub += QStringLiteral("...");

        auto *item = new QListWidgetItem();
        item->setData(RoleId, todo.getId());
        item->setData(RoleTitle, todo.getTitle());
        item->setData(RoleSubText, sub);
        item->setData(RoleDate, todo.getCreatedTime().toString(QStringLiteral("MM-dd")));
        item->setData(RoleColor, todo.getTagColor());
        item->setData(RoleCompleted, todo.isCompleted());
        item->setData(RolePriority, todo.getPriority());
        item->setData(RolePinned, todo.isPinned());
        m_todoList->addItem(item);

        if (todo.getId() == m_currentItemId) {
            selectRow = i;
        }
    }
    m_todoList->blockSignals(false);

    if (selectRow >= 0) {
        m_todoList->setCurrentRow(selectRow);
    }

    m_todoEmptyHint->setText(QStringLiteral("这个文件夹还是空的\n在上方输入框快速创建第一条待办吧"));
    m_todoEmptyHint->setVisible(items.isEmpty());
}

void MainWindow::updateDetailPanel()
{
    TodoItem *item = currentItem();
    if (!item) {
        clearDetailPanel();
        return;
    }

    m_emptyStateLabel->setVisible(false);

    m_titleEdit->setText(item->getTitle());
    m_detailsEdit->setPlainText(item->getDetails());
    m_createdTimeLabel->setText(item->getCreatedTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    bool showCompleted = item->isCompleted() && !item->getCompletedTime().isNull();
    m_completedTimeTitle->setVisible(showCompleted);
    m_completedTimeLabel->setVisible(showCompleted);
    if (showCompleted) {
        m_completedTimeLabel->setText(item->getCompletedTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    }

    m_completedCheck->blockSignals(true);
    m_completedCheck->setChecked(item->isCompleted());
    m_completedCheck->blockSignals(false);

    m_remindCheck->blockSignals(true);
    m_remindCheck->setChecked(item->getRemindAt().isValid());
    m_remindCheck->blockSignals(false);

    m_priorityCombo->setCurrentIndex(qBound(0, item->getPriority(), 2));

    const QStringList colors = Theme::palette();
    int colorIndex = qMax(0, colors.indexOf(item->getTagColor()));
    m_tagColorCombo->setCurrentIndex(colorIndex);

    updateTodoTags();

    for (QWidget *w : {static_cast<QWidget*>(m_titleEdit), static_cast<QWidget*>(m_detailsEdit),
                       static_cast<QWidget*>(m_completedCheck), static_cast<QWidget*>(m_remindCheck),
                       static_cast<QWidget*>(m_saveBtn),
                       static_cast<QWidget*>(m_deleteBtn), static_cast<QWidget*>(m_priorityCombo),
                       static_cast<QWidget*>(m_tagColorCombo), static_cast<QWidget*>(m_addTagBtn)}) {
        w->setEnabled(true);
    }

    // 详情面板淡入动效
    auto *effect = new QGraphicsOpacityEffect(m_detailCard);
    m_detailCard->setGraphicsEffect(effect);
    auto *anim = new QPropertyAnimation(effect, "opacity", m_detailCard);
    anim->setDuration(220);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
    connect(anim, &QPropertyAnimation::finished, m_detailCard, [this]() {
        m_detailCard->setGraphicsEffect(nullptr);
    });
}

void MainWindow::clearDetailPanel()
{
    m_emptyStateLabel->setVisible(true);

    m_titleEdit->clear();
    m_detailsEdit->clear();
    m_createdTimeLabel->setText(QStringLiteral("-"));
    m_completedTimeTitle->setVisible(false);
    m_completedTimeLabel->setVisible(false);

    m_completedCheck->blockSignals(true);
    m_completedCheck->setChecked(false);
    m_completedCheck->blockSignals(false);

    m_remindCheck->blockSignals(true);
    m_remindCheck->setChecked(false);
    m_remindCheck->blockSignals(false);

    m_priorityCombo->setCurrentIndex(0);
    m_tagColorCombo->setCurrentIndex(0);
    m_tagsDisplayLabel->setText(QStringLiteral("无标签"));

    for (QWidget *w : {static_cast<QWidget*>(m_titleEdit), static_cast<QWidget*>(m_detailsEdit),
                       static_cast<QWidget*>(m_completedCheck), static_cast<QWidget*>(m_remindCheck),
                       static_cast<QWidget*>(m_saveBtn),
                       static_cast<QWidget*>(m_deleteBtn), static_cast<QWidget*>(m_priorityCombo),
                       static_cast<QWidget*>(m_tagColorCombo), static_cast<QWidget*>(m_addTagBtn)}) {
        w->setEnabled(false);
    }
}

void MainWindow::updateTodoTags()
{
    TodoItem *item = currentItem();
    if (!item) {
        m_tagsDisplayLabel->setText(QStringLiteral("无标签"));
        return;
    }

    QStringList tags = item->getTags();
    if (tags.isEmpty()) {
        m_tagsDisplayLabel->setText(QStringLiteral("无标签"));
        m_tagsDisplayLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;")
                                          .arg(Theme::textMuted().name()));
    } else {
        m_tagsDisplayLabel->setText(tags.join(QStringLiteral("  ·  ")));
        m_tagsDisplayLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;")
                                          .arg(Theme::primary().name()));
    }
}

void MainWindow::updateStatusBar()
{
    int total = 0, done = 0;
    for (const TodoFolder &folder : m_folders) {
        total += folder.getItemCount();
        done += folder.getCompletedCount();
    }
    m_statusLabel->setText(QStringLiteral("共 %1 个文件夹 · %2 项待办 · 已完成 %3 项 · 数据已自动备份")
                           .arg(m_folders.size()).arg(total).arg(done));
}

void MainWindow::refreshAllViews()
{
    updateFolderList();
    updateTodoList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
    updateStatsWidget();
    updateStatusBar();
}

void MainWindow::updateStatsWidget()
{
    if (m_statsWidget) {
        m_statsWidget->setData(m_folders);
    }
}

// ==========================================================
// 全局搜索
// ==========================================================

void MainWindow::onSearchTextChanged(const QString &text)
{
    const QString needle = text.trimmed();
    if (needle.isEmpty()) {
        if (m_searching) {
            m_searching = false;
            updateTodoList();
        }
        return;
    }
    m_searching = true;
    showSearchResults(needle);
    // 切到列表页展示结果
    if (m_stack->currentIndex() != 0) {
        m_navBar->setCurrentIndex(0);
    }
}

void MainWindow::showSearchResults(const QString &text)
{
    m_todoList->blockSignals(true);
    m_todoList->clear();

    int hits = 0;
    for (const TodoFolder &folder : m_folders) {
        for (const TodoItem &todo : folder.getItems()) {
            const bool match = todo.getTitle().contains(text, Qt::CaseInsensitive)
                || todo.getDetails().contains(text, Qt::CaseInsensitive)
                || todo.getTags().contains(text, Qt::CaseInsensitive);
            if (!match) continue;

            auto *item = new QListWidgetItem();
            item->setData(RoleId, todo.getId());
            item->setData(RoleFolderId, folder.getId());
            item->setData(RoleTitle, todo.getTitle());
            item->setData(RoleSubText, QStringLiteral("%1 · %2").arg(folder.getName(),
                          todo.getDetails().split('\n').first().left(30)));
            item->setData(RoleDate, todo.getCreatedTime().toString(QStringLiteral("MM-dd")));
            item->setData(RoleColor, todo.getTagColor());
            item->setData(RoleCompleted, todo.isCompleted());
            item->setData(RolePriority, todo.getPriority());
            item->setData(RolePinned, todo.isPinned());
            m_todoList->addItem(item);
            ++hits;
        }
    }
    m_todoList->blockSignals(false);

    m_todoHeader->setTitle(QStringLiteral("搜索：%1").arg(text));
    m_todoEmptyHint->setText(QStringLiteral("没有找到包含 \"%1\" 的待办").arg(text));
    m_todoEmptyHint->setVisible(hits == 0);
}

// ==========================================================
// 回收站
// ==========================================================

void MainWindow::onTrashClicked()
{
    auto &db = DatabaseManager::instance();
    const QList<TodoItem> deleted = db.loadDeleted();

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("回收站"));
    dlg.resize(480, 420);
    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto *hint = new QLabel(QStringLiteral("已删除的待办保留 30 天，之后自动清除。"), &dlg);
    hint->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;").arg(Theme::textMuted().name()));
    layout->addWidget(hint);

    auto *list = new QListWidget(&dlg);
    for (const TodoItem &item : deleted) {
        const QString folderName = db.deletedItemFolderName(item.getFolderId());
        auto *li = new QListWidgetItem(QStringLiteral("%1    （原文件夹：%2）")
                                       .arg(item.getTitle(),
                                            folderName.isEmpty() ? QStringLiteral("已删除") : folderName));
        li->setData(Qt::UserRole, item.getId());
        if (item.isCompleted()) {
            li->setForeground(Theme::textMuted());
        }
        list->addItem(li);
    }
    if (deleted.isEmpty()) {
        auto *li = new QListWidgetItem(QStringLiteral("回收站是空的"));
        li->setFlags(Qt::NoItemFlags);
        li->setForeground(Theme::textMuted());
        list->addItem(li);
    }
    layout->addWidget(list, 1);

    auto *btnRow = new QHBoxLayout();
    auto *restoreBtn = new QPushButton(QStringLiteral("恢复到原文件夹"), &dlg);
    restoreBtn->setProperty("variant", "primary");
    auto *hardDeleteBtn = new QPushButton(QStringLiteral("彻底删除"), &dlg);
    hardDeleteBtn->setProperty("variant", "danger");
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), &dlg);
    btnRow->addWidget(restoreBtn, 1);
    btnRow->addWidget(hardDeleteBtn, 1);
    btnRow->addWidget(closeBtn, 1);
    layout->addLayout(btnRow);

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(restoreBtn, &QPushButton::clicked, &dlg, [this, &db, &dlg, list]() {
        QListWidgetItem *cur = list->currentItem();
        if (!cur || cur->data(Qt::UserRole).isNull()) {
            MessageUtils::showInfo(&dlg, QStringLiteral("提示"), QStringLiteral("请先选择要恢复的待办。"));
            return;
        }
        if (!db.restoreItem(cur->data(Qt::UserRole).toString())) {
            MessageUtils::showInfo(&dlg, QStringLiteral("无法恢复"),
                                   QStringLiteral("原文件夹已被删除，该待办无法恢复。"));
            return;
        }
        // 重新加载内存数据并刷新
        m_folders = db.loadAll();
        refreshAllViews();
        clearDetailPanel();
        dlg.accept();
        MessageUtils::showSuccess(this, QStringLiteral("已恢复"), QStringLiteral("待办已恢复到原文件夹。"));
    });
    connect(hardDeleteBtn, &QPushButton::clicked, &dlg, [this, &db, &dlg, list]() {
        QListWidgetItem *cur = list->currentItem();
        if (!cur || cur->data(Qt::UserRole).isNull()) {
            MessageUtils::showInfo(&dlg, QStringLiteral("提示"), QStringLiteral("请先选择要彻底删除的待办。"));
            return;
        }
        if (!MessageUtils::showConfirm(&dlg, QStringLiteral("彻底删除"),
                QStringLiteral("彻底删除后无法恢复，确定继续吗？"))) {
            return;
        }
        db.hardDeleteItem(cur->data(Qt::UserRole).toString());
        delete list->takeItem(list->row(cur));
    });

    dlg.exec();
}

// ==========================================================
// 到期提醒
// ==========================================================

void MainWindow::checkReminders()
{
    if (!m_trayIcon || !m_trayIcon->isVisible()) return;

    const QDateTime now = QDateTime::currentDateTime();
    for (TodoFolder &folder : m_folders) {
        for (TodoItem &item : folder.getItemsRef()) {
            const QDateTime remindAt = item.getRemindAt();
            if (!remindAt.isValid() || item.isCompleted() || remindAt > now) {
                continue;
            }
            m_trayIcon->showMessage(QStringLiteral("待办提醒"),
                                    QStringLiteral("「%1」今天到期，记得处理哦").arg(item.getTitle()),
                                    QSystemTrayIcon::Information, 8000);
            item.setRemindAt(QDateTime());   // 提醒一次后清除
            persistItem(&item);
        }
    }
}

// ==========================================================
// 深色模式
// ==========================================================

void MainWindow::onToggleDarkMode(bool dark)
{
    Theme::setDark(dark);
    QSettings settings;
    settings.setValue(QStringLiteral("ui/darkMode"), dark);

    QFile f(dark ? QStringLiteral(":/styles-dark.qss") : QStringLiteral(":/styles.qss"));
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream ts(&f);
        qApp->setStyleSheet(ts.readAll());
        f.close();
    }

    // 重新应用构建期烘焙的样式（玻璃面板、空状态、分节标签等）
    if (QWidget *p = findChild<QWidget*>(QStringLiteral("folderPanel"))) p->setStyleSheet(Theme::glassPanelStyle(QStringLiteral("folderPanel")));
    if (QWidget *p = findChild<QWidget*>(QStringLiteral("todoPanel")))   p->setStyleSheet(Theme::glassPanelStyle(QStringLiteral("todoPanel")));
    if (QWidget *p = findChild<QWidget*>(QStringLiteral("detailPanel"))) p->setStyleSheet(Theme::glassPanelStyle(QStringLiteral("detailPanel")));

    const QString hintStyle = QStringLiteral("color: %1; font-size: 13px; padding: 20px;")
                              .arg(Theme::textMuted().name());
    m_folderEmptyHint->setStyleSheet(hintStyle);
    m_todoEmptyHint->setStyleSheet(hintStyle);
    m_emptyStateLabel->setStyleSheet(
        QStringLiteral("color: %1; font-size: 13px; padding: 28px 16px; background-color: %2; border: 1px dashed %4; border-radius: %3px;")
        .arg(Theme::textMuted().name(), Theme::glassBg().name(QColor::HexArgb))
        .arg(Theme::radiusMd).arg(Theme::glassBorder().name(QColor::HexArgb)));

    const QString labelStyle = QStringLiteral("color: %1; font-size: 12px; font-weight: 500;")
                               .arg(Theme::textSecondary().name());
    for (QLabel *l : m_detailCard->findChildren<QLabel*>()) {
        if (l->property("sectionLabel").toBool()) {
            l->setStyleSheet(labelStyle);
        }
    }
    const QString valueStyle = QStringLiteral("color: %1; font-size: 13px;")
                               .arg(Theme::textPrimary().name());
    m_createdTimeLabel->setStyleSheet(valueStyle);
    m_completedTimeLabel->setStyleSheet(valueStyle);
    updateTodoTags();

    // 日历 / 标签页重建样式表（内部自绘色随 paintEvent 自动适配）
    if (m_calendarWidget) m_calendarWidget->refreshTheme();
    if (m_tagWidget)      m_tagWidget->refreshTheme();

    // 自绘控件（标题栏/导航栏/列表代理/日历/标签/统计）统一重绘
    for (QWidget *w : QApplication::topLevelWidgets()) {
        w->update();
    }
    refreshAllViews();
}

// ==========================================================
// 子视图集成
// ==========================================================

void MainWindow::setupDesktopWidget()
{
    m_desktopWidget = new DesktopWidget();
    m_desktopWidget->updateTodoData(m_folders);

    connect(m_desktopWidget, &DesktopWidget::newTodoRequested, this, &MainWindow::onDesktopNewTodo);
    connect(m_desktopWidget, &DesktopWidget::todoItemToggled, this, &MainWindow::onDesktopTodoToggled);
    connect(m_desktopWidget, &DesktopWidget::showMainWindowRequested, this, &MainWindow::onShowMainWindow);
    connect(m_desktopWidget, &DesktopWidget::editTodoRequested, this, [this](const QString &itemId) {
        QString folderId;
        if (findTodoItemById(itemId, &folderId)) {
            m_currentFolderId = folderId;
            m_currentItemId = itemId;
            m_navBar->setCurrentIndex(0);
            onShowMainWindow();
            updateFolderList();
            updateTodoList();
            updateDetailPanel();
        }
    });
    connect(m_desktopWidget, &DesktopWidget::deleteTodoRequested, this, [this](const QString &itemId) {
        deleteTodoItem(itemId);
    });
}

void MainWindow::updateDesktopWidget()
{
    if (m_desktopWidget) {
        m_desktopWidget->updateTodoData(m_folders);
    }
}

void MainWindow::onDesktopNewTodo(const QString &title)
{
    // 桌面快速添加：归入以今天日期命名的文件夹
    QString todayName = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));

    TodoFolder *target = nullptr;
    for (TodoFolder &folder : m_folders) {
        if (folder.getName() == todayName) {
            target = &folder;
            break;
        }
    }
    if (!target) {
        m_folders.append(TodoFolder(todayName));
        target = &m_folders.last();
        persistFolder(target);
    }

    TodoItem item(title);
    QString folderId = target->getId();
    target->addItem(item);
    if (TodoItem *stored = target->findItem(item.getId())) {
        persistItem(stored);
    }

    Q_UNUSED(folderId)
    refreshAllViews();
}

void MainWindow::onDesktopTodoToggled(const QString &itemId, bool completed)
{
    toggleTodoCompleted(itemId, completed);
}

void MainWindow::onShowMainWindow()
{
    show();
    activateWindow();
    raise();
}

void MainWindow::setupCalendarWidget()
{
    m_calendarWidget = new CalendarWidget(this);

    connect(m_calendarWidget, &CalendarWidget::todoItemAdded, this, &MainWindow::onCalendarTodoAdded);
    connect(m_calendarWidget, &CalendarWidget::todoItemToggled, this, &MainWindow::onCalendarTodoToggled);
    connect(m_calendarWidget, &CalendarWidget::todoItemDeleted, this, &MainWindow::onCalendarTodoDeleted);
}

void MainWindow::updateCalendarWidget()
{
    if (m_calendarWidget) {
        m_calendarWidget->updateTodoData(m_folders);
    }
}

void MainWindow::onCalendarTodoAdded(const QString &title, const QDate &date)
{
    QString folderName = date.toString(QStringLiteral("yyyy-MM-dd"));

    TodoFolder *target = nullptr;
    for (TodoFolder &folder : m_folders) {
        if (folder.getName() == folderName) {
            target = &folder;
            break;
        }
    }
    if (!target) {
        m_folders.append(TodoFolder(folderName));
        target = &m_folders.last();
        persistFolder(target);
    }

    TodoItem item(title);
    item.setDueDate(date);
    item.setPlannedDate(date);
    target->addItem(item);
    if (TodoItem *stored = target->findItem(item.getId())) {
        persistItem(stored);
    }

    refreshAllViews();
}

void MainWindow::onCalendarTodoToggled(const QString &itemId, bool completed)
{
    toggleTodoCompleted(itemId, completed);
}

void MainWindow::onCalendarTodoDeleted(const QString &itemId)
{
    deleteTodoItem(itemId);
}

void MainWindow::setupTagWidget()
{
    m_tagWidget = new TagWidget(this);

    connect(m_tagWidget, &TagWidget::todoClicked, this, [this](const QString &todoId, const QString &folderId) {
        if (findTodoItemById(todoId)) {
            m_currentFolderId = folderId;
            m_currentItemId = todoId;
            updateFolderList();
            updateTodoList();
            updateDetailPanel();
            m_navBar->setCurrentIndex(0);
        }
    });
    connect(m_tagWidget, &TagWidget::todoToggled, this, [this](const QString &todoId, bool completed) {
        toggleTodoCompleted(todoId, completed);
    });
    connect(m_tagWidget, &TagWidget::tagCreated, this, [this](const QString &tag) {
        DatabaseManager::instance().addTag(tag);
        updateTagWidget();
    });
    connect(m_tagWidget, &TagWidget::tagDeleted, this, [this](const QString &tag) {
        DatabaseManager::instance().removeTag(tag);
        for (TodoFolder &folder : m_folders) {
            for (TodoItem &item : folder.getItems()) {
                item.removeTag(tag);
            }
        }
        updateTodoList();
        updateTagWidget();
        updateTodoTags();
    });
}

void MainWindow::updateTagWidget()
{
    if (m_tagWidget) {
        m_tagWidget->updateData(m_folders, DatabaseManager::instance().allTagNames());
    }
}

// ==========================================================
// 文件 / 数据
// ==========================================================

void MainWindow::onImportClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, QStringLiteral("导入数据"), QString(),
                                                    QStringLiteral("JSON文件 (*.json)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        MessageUtils::showError(this, QStringLiteral("错误"), QStringLiteral("无法打开文件进行读取。"));
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject() || !doc.object().contains(QStringLiteral("folders"))) {
        MessageUtils::showError(this, QStringLiteral("错误"), QStringLiteral("文件格式不正确，缺少 folders 数据。"));
        return;
    }

    // 先在内存中完整解析并校验，避免半途失败损坏现有数据
    QList<TodoFolder> imported;
    QJsonArray foldersArray = doc.object()[QStringLiteral("folders")].toArray();
    for (const QJsonValue &folderVal : foldersArray) {
        if (!folderVal.isObject()) {
            MessageUtils::showError(this, QStringLiteral("错误"), QStringLiteral("文件内容损坏，导入已取消。"));
            return;
        }
        imported.append(TodoFolder(folderVal.toObject()));
    }

    if (!MessageUtils::showConfirm(this, QStringLiteral("确认导入"),
            QStringLiteral("导入将替换当前全部数据（%1 个文件夹）。\n系统会先自动备份当前数据，确定继续吗？").arg(imported.size()))) {
        return;
    }

    // 导入前自动备份
    DatabaseManager::instance().backupNow(QStringLiteral("pre_import"));

    if (!DatabaseManager::instance().replaceAll(imported)) {
        MessageUtils::showError(this, QStringLiteral("导入失败"),
                                DatabaseManager::instance().lastError() + QStringLiteral("\n现有数据未受影响。"));
        return;
    }

    m_folders = imported;
    m_currentFolderId.clear();
    m_currentItemId.clear();

    refreshAllViews();
    clearDetailPanel();
    if (!m_folders.isEmpty()) {
        m_folderList->setCurrentRow(0);
    }

    MessageUtils::showSuccess(this, QStringLiteral("导入成功"), QStringLiteral("数据已成功导入！"));
}

void MainWindow::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("导出数据"),
                                                    QStringLiteral("todolist_export.json"),
                                                    QStringLiteral("JSON文件 (*.json)"));
    if (fileName.isEmpty()) return;

    QJsonObject root;
    QJsonArray foldersArray;
    for (const TodoFolder &folder : m_folders) {
        foldersArray.append(folder.toJson());
    }
    root[QStringLiteral("folders")] = foldersArray;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        MessageUtils::showError(this, QStringLiteral("错误"), QStringLiteral("无法创建文件进行写入。"));
        return;
    }
    file.write(QJsonDocument(root).toJson());
    file.close();

    MessageUtils::showSuccess(this, QStringLiteral("导出成功"), QStringLiteral("数据已成功导出！"));
}

void MainWindow::onBackupClicked()
{
    if (DatabaseManager::instance().backupNow(QStringLiteral("manual"))) {
        MessageUtils::showSuccess(this, QStringLiteral("备份完成"),
                                  QStringLiteral("数据库快照已保存到备份目录。"));
    } else {
        MessageUtils::showError(this, QStringLiteral("备份失败"), DatabaseManager::instance().lastError());
    }
}

void MainWindow::onOpenBackupDir()
{
    QDir().mkpath(DatabaseManager::instance().backupDir());
    QDesktopServices::openUrl(QUrl::fromLocalFile(DatabaseManager::instance().backupDir()));
}

void MainWindow::onExitClicked()
{
    if (m_desktopWidget) {
        m_desktopWidget->close();
    }
    qApp->quit();
}

void MainWindow::onDesktopWidgetClicked()
{
    if (m_desktopWidget) {
        m_desktopWidget->setVisible(!m_desktopWidget->isVisible());
        if (m_desktopWidget->isVisible()) {
            m_desktopWidget->activateWindow();
        }
    }
}

// ==========================================================
// 托盘 / 生命周期
// ==========================================================

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);
    QIcon trayIcon(QStringLiteral(":/icons/app.ico"));
    if (trayIcon.isNull()) {
        trayIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    m_trayIcon->setIcon(trayIcon);
    m_trayIcon->setToolTip(QStringLiteral("Todo List - 待办事项管理"));

    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction(QStringLiteral("显示主窗口"), this, &MainWindow::onShowFromTray);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(QStringLiteral("退出"), this, &MainWindow::onExitFromTray);
    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    m_trayIcon->show();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        onShowFromTray();
    }
}

void MainWindow::onShowFromTray()
{
    show();
    activateWindow();
    raise();
}

void MainWindow::onExitFromTray()
{
    if (m_desktopWidget) {
        m_desktopWidget->close();
    }
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSplitterState();
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::onAboutToQuit()
{
    saveSplitterState();
    if (m_desktopWidget) {
        m_desktopWidget->close();
    }
}

// ==========================================================
// 拖拽：待办事项 -> 文件夹
// ==========================================================

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_folderList->viewport()) {
        if (event->type() == QEvent::DragEnter) {
            auto *e = static_cast<QDragEnterEvent*>(event);
            if (e->source() == m_todoList) {
                e->accept();
                return true;
            }
        } else if (event->type() == QEvent::DragMove) {
            auto *e = static_cast<QDragMoveEvent*>(event);
            QListWidgetItem *target = m_folderList->itemAt(e->position().toPoint());
            if (target != m_dragHoverItem) {
                m_dragHoverItem = target;
                m_folderList->viewport()->update();
            }
            e->accept();
            return true;
        } else if (event->type() == QEvent::DragLeave) {
            m_dragHoverItem = nullptr;
            m_folderList->viewport()->update();
            return true;
        } else if (event->type() == QEvent::Drop) {
            auto *e = static_cast<QDropEvent*>(event);
            QListWidgetItem *target = m_folderList->itemAt(e->position().toPoint());
            m_dragHoverItem = nullptr;

            if (target && !m_currentItemId.isEmpty() && !m_currentFolderId.isEmpty()) {
                QString targetFolderId = target->data(RoleId).toString();
                if (targetFolderId != m_currentFolderId) {
                    TodoFolder *src = currentFolder();
                    TodoFolder *dst = findFolderById(targetFolderId);
                    TodoItem *item = src ? src->findItem(m_currentItemId) : nullptr;
                    if (src && dst && item) {
                        TodoItem copy = *item;
                        src->removeItem(copy.getId());
                        dst->addItem(copy);
                        if (TodoItem *stored = dst->findItem(copy.getId())) {
                            DatabaseManager::instance().moveItem(stored->getId(), targetFolderId);
                        }

                        m_currentFolderId = targetFolderId;
                        m_currentItemId = copy.getId();
                        refreshAllViews();
                        updateDetailPanel();
                    }
                }
            }
            e->accept();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// ==========================================================
// 无边框窗口：原生边框缩放 + 最大化避让任务栏（Windows）
// ==========================================================

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG*>(message);

        if (msg->message == WM_GETMINMAXINFO) {
            // 无边框窗口最大化时限制在工作区内，避免遮挡任务栏
            HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi{};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfo(monitor, &mi)) {
                auto *mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
                mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
                mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
                mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
                mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
            }
            *result = 0;
            return true;
        }

        if (msg->message == WM_NCHITTEST) {
            if (isMaximized()) {
                *result = HTCLIENT;
                return true;
            }
            RECT rc{};
            GetWindowRect(msg->hwnd, &rc);
            const long x = GET_X_LPARAM(msg->lParam) - rc.left;
            const long y = GET_Y_LPARAM(msg->lParam) - rc.top;
            const long w = rc.right - rc.left;
            const long h = rc.bottom - rc.top;
            const int border = 6;

            const bool left   = x < border;
            const bool right  = x >= w - border;
            const bool top    = y < border;
            const bool bottom = y >= h - border;

            if (top && left)        *result = HTTOPLEFT;
            else if (top && right)  *result = HTTOPRIGHT;
            else if (bottom && left)*result = HTBOTTOMLEFT;
            else if (bottom && right)*result = HTBOTTOMRIGHT;
            else if (left)          *result = HTLEFT;
            else if (right)         *result = HTRIGHT;
            else if (top)           *result = HTTOP;
            else if (bottom)        *result = HTBOTTOM;
            else                    *result = HTCLIENT;
            return true;
        }
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}
