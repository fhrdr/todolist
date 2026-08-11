#include "desktopwidget.h"
#include "../icons.h"
#include "../theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QCloseEvent>
#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QMenu>
#include <QCursor>
#include <QDate>
#include <QStringList>
#include <QRandomGenerator>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QUrl>
#include <algorithm>
#include <cmath>

namespace {

// ---- 列表数据角色 ----
constexpr int RoleId        = Qt::UserRole;
constexpr int RoleCompleted = Qt::UserRole + 1;
constexpr int RolePriority  = Qt::UserRole + 2;
constexpr int RoleColor     = Qt::UserRole + 3;
constexpr int RoleDueText   = Qt::UserRole + 4;   // 倒数日文案（"剩 3 天"）
constexpr int RoleDueUrgent = Qt::UserRole + 5;   // 倒数日是否紧急（今天/过期）

constexpr int kShadowMargin = 12;   // 纸张外圈阴影留白

// 纸张配色（顶色 / 底色 / 强调色）
struct PaperColors { QColor top, bottom, accent; };
PaperColors paperColors(int theme)
{
    switch (theme) {
    case DesktopWidget::Sakura: return { QColor(0xFF, 0xE4, 0xEC), QColor(0xFF, 0xCF, 0xE0), QColor(0xEC, 0x40, 0x7A) };
    case DesktopWidget::Mint:   return { QColor(0xE0, 0xF5, 0xE9), QColor(0xC6, 0xED, 0xD6), QColor(0x26, 0xA6, 0x9A) };
    case DesktopWidget::Sky:    return { QColor(0xE1, 0xF0, 0xFA), QColor(0xC8, 0xE2, 0xF5), QColor(0x42, 0xA5, 0xF5) };
    case DesktopWidget::Cream:  return { QColor(0xFF, 0xFD, 0xF4), QColor(0xFB, 0xF3, 0xDC), QColor(0xC0, 0xA0, 0x62) };
    default:                    return { QColor(0xFF, 0xF9, 0xC4), QColor(0xFF, 0xEC, 0xA0), QColor(0xF9, 0xA8, 0x25) };
    }
}

const QColor kTextWarm   (0x5D, 0x40, 0x37);   // 纸张上的暖棕文字
const QColor kTextSoft   (0x8D, 0x6E, 0x63);
const QColor kLineBrown  (0x7A, 0x5C, 0x4F);

// ---- 每日一句 ----
const QStringList kDailyQuotes = {
    QStringLiteral("把每一件小事做好，就是不平凡。"),
    QStringLiteral("今天也要元气满满哦。"),
    QStringLiteral("慢慢来，反而比较快。"),
    QStringLiteral("完成比完美更重要。"),
    QStringLiteral("你认真生活的样子，真好看。"),
    QStringLiteral("别急，一切都在慢慢变好。"),
    QStringLiteral("此刻的努力，未来都会记得。"),
    QStringLiteral("喝口水，伸个懒腰，再出发。"),
    QStringLiteral("一步一步走，总会到达。"),
    QStringLiteral("今天的你，比昨天更棒了一点。"),
    QStringLiteral("心有暖阳，何惧路长。"),
    QStringLiteral("把烦恼折成纸飞机，轻轻放飞。"),
    QStringLiteral("认真做好眼前事，好运自然来。"),
    QStringLiteral("累的时候，别忘了夸夸自己。"),
    QStringLiteral("生活不在别处，就在当下。"),
    QStringLiteral("小小的进步，也值得庆祝。")
};

// ---- 天气图标类别 ----
enum WeatherKind { WSun = 0, WPartly, WCloud, WRain, WSnow, WThunder, WFog };

// wttr.in weatherCode -> 类别
int weatherKind(int code)
{
    switch (code) {
    case 113: return WSun;
    case 116: return WPartly;
    case 119: case 122: return WCloud;
    case 143: case 248: case 260: return WFog;
    case 200: case 386: case 389: case 392: return WThunder;
    case 227: case 230: case 320: case 323: case 326: case 329: case 332:
    case 335: case 338: case 350: case 368: case 371: case 374: case 377:
    case 395: return WSnow;
    default:  return WRain;
    }
}

// 云朵轮廓（多个圆 + 底座）
QPainterPath cloudPath(const QRectF &r)
{
    QPainterPath p;
    p.addEllipse(r.left(), r.top() + r.height() * 0.30, r.width() * 0.52, r.height() * 0.52);
    p.addEllipse(r.left() + r.width() * 0.26, r.top(), r.width() * 0.56, r.height() * 0.78);
    p.addEllipse(r.left() + r.width() * 0.52, r.top() + r.height() * 0.26, r.width() * 0.46, r.height() * 0.56);
    p.addRect(QRectF(r.left() + 1, r.top() + r.height() * 0.52, r.width() - 2, r.height() * 0.42));
    return p.simplified();
}

// ---- 便利贴列表代理：手绘圆圈复选框 + 标题 + 倒数日 ----
class StickyNoteDelegate : public QStyledItemDelegate
{
public:
    explicit StickyNoteDelegate(QListWidget *view) : QStyledItemDelegate(view) {}

    QColor accent = QColor(0xF9, 0xA8, 0x25);

    static QRect checkRect(const QRect &itemRect)
    {
        return QRect(itemRect.left() + 4, itemRect.center().y() - 11, 24, 22);
    }

    void paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);

        const bool hovered  = option.state & QStyle::State_MouseOver;
        const bool completed = index.data(RoleCompleted).toBool();
        const int priority   = index.data(RolePriority).toInt();
        const QString tagColor = index.data(RoleColor).toString();
        const QString title  = index.data(Qt::DisplayRole).toString();
        const QString due    = index.data(RoleDueText).toString();
        const bool dueUrgent = index.data(RoleDueUrgent).toBool();

        // 悬停背景
        if (hovered && !completed) {
            p->setPen(Qt::NoPen);
            p->setBrush(QColor(255, 255, 255, 110));
            p->drawRoundedRect(option.rect.adjusted(2, 2, -2, -2), 8, 8);
        }

        int x = option.rect.left() + 4;
        const int cy = option.rect.center().y();

        // 手绘圆圈复选框
        QRect circle(x + 2, cy - 8, 16, 16);
        if (completed) {
            p->setPen(Qt::NoPen);
            p->setBrush(accent);
            p->drawEllipse(circle);
            QPainterPath check;
            check.moveTo(circle.left() + 4.5, circle.center().y());
            check.lineTo(circle.center().x() - 0.5, circle.bottom() - 4.5);
            check.lineTo(circle.right() - 4, circle.top() + 5);
            p->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p->setBrush(Qt::NoBrush);
            p->drawPath(check);
        } else {
            p->setPen(QPen(hovered ? accent : kTextSoft, 1.8));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(circle);
        }
        x += 26;

        // 优先级点
        if (priority > 0 && !completed) {
            p->setPen(Qt::NoPen);
            p->setBrush(priority == 2 ? QColor(0xE5, 0x39, 0x35) : QColor(0xFB, 0x8C, 0x00));
            p->drawEllipse(x, cy - 3, 6, 6);
            x += 11;
        }

        // 标签色点
        if (!tagColor.isEmpty()) {
            p->setPen(Qt::NoPen);
            QColor c(tagColor);
            if (completed) c.setAlpha(110);
            p->setBrush(c);
            p->drawEllipse(x, cy - 3, 6, 6);
            x += 11;
        }

        // 倒数日（右侧）
        int titleRight = option.rect.right() - 8;
        if (!due.isEmpty()) {
            QFont dueFont(QStringLiteral("Microsoft YaHei UI"), -1);
            dueFont.setPixelSize(11);
            QFontMetrics dfm(dueFont);
            int dw = dfm.horizontalAdvance(due) + 12;
            QRect dueRect(titleRight - dw, cy - 9, dw, 18);
            p->setPen(Qt::NoPen);
            p->setBrush(dueUrgent ? QColor(0xE5, 0x39, 0x35, 26) : QColor(0x7A, 0x5C, 0x4F, 18));
            p->drawRoundedRect(dueRect, 9, 9);
            p->setFont(dueFont);
            p->setPen(dueUrgent ? QColor(0xE5, 0x39, 0x35) : kTextSoft);
            p->drawText(dueRect, Qt::AlignCenter, due);
            titleRight = dueRect.left() - 6;
        }

        // 标题
        QFont titleFont(QStringLiteral("Microsoft YaHei UI"), -1);
        titleFont.setPixelSize(14);
        titleFont.setStrikeOut(completed);
        p->setFont(titleFont);
        p->setPen(completed ? QColor(0xB0, 0x9C, 0x92) : kTextWarm);
        QFontMetrics tfm(titleFont);
        int maxW = qMax(titleRight - x, 20);
        p->drawText(QRect(x, option.rect.top(), maxW, option.rect.height()),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    tfm.elidedText(title, Qt::ElideRight, maxW));

        p->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(200, 34);
    }
};

} // namespace

// ==========================================================
// 构造 / 析构
// ==========================================================

DesktopWidget::DesktopWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(280, 340);

    loadAppearance();
    loadWindowSize();
    loadWindowPosition();
    applyTheme();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(60000);
    connect(m_refreshTimer, &QTimer::timeout, this, &DesktopWidget::onRefreshTimer);
    m_refreshTimer->start();

    // 天气：立即获取 + 每 30 分钟自动刷新
    m_netManager = new QNetworkAccessManager(this);
    m_weatherTimer = new QTimer(this);
    m_weatherTimer->setInterval(30 * 60 * 1000);
    connect(m_weatherTimer, &QTimer::timeout, this, &DesktopWidget::fetchWeather);
    m_weatherTimer->start();
    fetchWeather();

    updateHeader();
    scheduleBlink();

    setMouseTracking(true);
    m_todoListWidget->setMouseTracking(true);
    m_todoListWidget->viewport()->setMouseTracking(true);
    m_todoListWidget->viewport()->installEventFilter(this);
}

DesktopWidget::~DesktopWidget()
{
    saveWindowPosition();
    saveWindowSize();
    saveAppearance();
}

// ==========================================================
// UI 构建
// ==========================================================

void DesktopWidget::setupUI()
{
    // 纸张外侧留出阴影空间；顶部多留空间给胶带
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(kShadowMargin + 8, kShadowMargin + 22,
                                   kShadowMargin + 8, kShadowMargin + 8);
    mainLayout->setSpacing(4);

    // 头部：日期 + 置顶按钮
    auto *headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(2, 0, 0, 0);
    headerRow->setSpacing(4);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setMinimumWidth(1);
    headerRow->addWidget(m_dateLabel, 1);

    headerRow->addSpacing(104);   // 预留给天气贴片

    m_pinButton = new QPushButton(this);
    m_pinButton->setFixedSize(26, 26);
    m_pinButton->setCursor(Qt::PointingHandCursor);
    m_pinButton->setToolTip(QStringLiteral("置顶 / 取消置顶"));
    headerRow->addWidget(m_pinButton);
    mainLayout->addLayout(headerRow);

    m_countLabel = new QLabel(this);
    mainLayout->addWidget(m_countLabel);

    // 每日一句（左） + 倒计时贴片（右）
    auto *quoteRow = new QHBoxLayout();
    quoteRow->setContentsMargins(2, 0, 0, 0);
    quoteRow->setSpacing(6);
    m_quoteLabel = new QLabel(this);
    m_quoteLabel->setMinimumWidth(1);
    m_quoteLabel->setCursor(Qt::PointingHandCursor);
    m_quoteLabel->setToolTip(QStringLiteral("点我换一句"));
    m_quoteLabel->installEventFilter(this);
    quoteRow->addWidget(m_quoteLabel, 1);
    m_dueLabel = new QLabel(this);
    m_dueLabel->setMinimumWidth(1);
    m_dueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_dueLabel->setVisible(false);
    quoteRow->addWidget(m_dueLabel);
    mainLayout->addLayout(quoteRow);
    mainLayout->addSpacing(4);

    // 待办列表
    m_todoListWidget = new QListWidget(this);
    m_todoListWidget->setItemDelegate(new StickyNoteDelegate(m_todoListWidget));
    m_todoListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_todoListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_todoListWidget->setFrameShape(QFrame::NoFrame);
    mainLayout->addWidget(m_todoListWidget, 1);

    // 快速添加（右侧留白给卡通猫）
    auto *addRow = new QHBoxLayout();
    addRow->setContentsMargins(2, 0, 0, 0);
    addRow->setSpacing(0);
    m_addLineEdit = new QLineEdit(this);
    m_addLineEdit->setPlaceholderText(QStringLiteral("+ 添加待办，回车保存"));
    addRow->addWidget(m_addLineEdit, 1);
    addRow->addSpacing(58);
    mainLayout->addLayout(addRow);
}

void DesktopWidget::setupConnections()
{
    connect(m_addLineEdit, &QLineEdit::returnPressed, this, &DesktopWidget::onAddTodoClicked);
    connect(m_todoListWidget, &QListWidget::itemClicked, this, &DesktopWidget::onTodoItemClicked);
    connect(m_pinButton, &QPushButton::clicked, this, [this]() { setAlwaysOnTop(!m_alwaysOnTop); });
}

// ==========================================================
// 主题 / 外观
// ==========================================================

QColor DesktopWidget::paperTop() const    { return paperColors(m_theme).top; }
QColor DesktopWidget::paperBottom() const { return paperColors(m_theme).bottom; }
QColor DesktopWidget::accentColor() const { return paperColors(m_theme).accent; }

void DesktopWidget::applyTheme()
{
    const QString textWarm = kTextWarm.name();
    const QString textSoft = kTextSoft.name();

    m_dateLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 15px; font-weight: 700; background: transparent;").arg(textWarm));
    m_countLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 11px; background: transparent;").arg(textSoft));
    m_quoteLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px; font-style: italic; background: transparent;").arg(textSoft));

    m_todoListWidget->setStyleSheet(QStringLiteral(
        "QListWidget { border: none; background: transparent; }"
        "QScrollBar:vertical { background: transparent; width: 6px; margin: 2px; }"
        "QScrollBar::handle:vertical { background: %1; border-radius: 3px; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
        .arg(QColor(kLineBrown.red(), kLineBrown.green(), kLineBrown.blue(), 70).name(QColor::HexArgb)));

    m_addLineEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { border: none; border-bottom: 1px dashed %1; background: transparent;"
        "            color: %2; font-size: 13px; padding: 6px 2px; }"
        "QLineEdit:focus { border-bottom: 1px solid %3; }")
        .arg(QColor(kTextSoft.red(), kTextSoft.green(), kTextSoft.blue(), 130).name(QColor::HexArgb),
             textWarm, accentColor().name()));

    m_pinButton->setStyleSheet(QStringLiteral(
        "QPushButton { border: none; background: transparent; border-radius: 13px; }"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.45); }"));
    m_pinButton->setIcon(Icons::icon(Icons::Pin, 14,
                                     m_alwaysOnTop ? accentColor()
                                                   : QColor(0xB0, 0xA3, 0x98)));

    if (auto *d = static_cast<StickyNoteDelegate*>(m_todoListWidget->itemDelegate())) {
        d->accent = accentColor();
    }
    m_todoListWidget->viewport()->update();
    updateHeader();   // 倒计时贴片颜色随主题刷新
    update();
}

void DesktopWidget::setPaperTheme(PaperTheme theme)
{
    m_theme = theme;
    applyTheme();
    saveAppearance();
}

void DesktopWidget::setCharacter(Character character)
{
    m_character = character;
    update();
    saveAppearance();
}

void DesktopWidget::setNoteOpacity(qreal opacity)
{
    setWindowOpacity(qBound(0.3, opacity, 1.0));
    saveAppearance();
}

void DesktopWidget::setAlwaysOnTop(bool onTop)
{
    m_alwaysOnTop = onTop;
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool;
    if (m_alwaysOnTop) flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    show();
    applyTheme();
    saveAppearance();
}

// ==========================================================
// 数据
// ==========================================================

void DesktopWidget::updateTodoData(const QList<TodoFolder> &folders)
{
    m_folders = folders;
    loadPendingItems();
    updateTodoList();
    updateHeader();
}

void DesktopWidget::loadPendingItems()
{
    m_displayItems.clear();

    for (const TodoFolder &folder : m_folders) {
        for (const TodoItem &item : folder.getItems()) {
            if (!item.isCompleted()) {
                m_displayItems.append(item);
            }
        }
    }

    // 置顶 > 倒数日紧迫 > 创建时间
    std::sort(m_displayItems.begin(), m_displayItems.end(), [](const TodoItem &a, const TodoItem &b) {
        if (a.isPinned() != b.isPinned()) return a.isPinned() > b.isPinned();
        const bool aDue = a.getDueDate().isValid(), bDue = b.getDueDate().isValid();
        if (aDue != bDue) return aDue > bDue;
        if (aDue && bDue && a.getDueDate() != b.getDueDate()) return a.getDueDate() < b.getDueDate();
        return a.getCreatedTime() > b.getCreatedTime();
    });
}

void DesktopWidget::updateTodoList()
{
    m_todoListWidget->clear();

    const QDate today = QDate::currentDate();
    for (const TodoItem &item : m_displayItems) {
        auto *listItem = new QListWidgetItem(item.getTitle());
        listItem->setData(RoleId, item.getId());
        listItem->setData(RoleCompleted, item.isCompleted());
        listItem->setData(RolePriority, item.getPriority());
        listItem->setData(RoleColor, item.getTagColor());

        // 倒数日
        if (item.getDueDate().isValid()) {
            const qint64 days = today.daysTo(item.getDueDate());
            QString text;
            bool urgent = false;
            if (days < 0)       { text = QStringLiteral("过期 %1 天").arg(-days); urgent = true; }
            else if (days == 0) { text = QStringLiteral("今天"); urgent = true; }
            else if (days == 1) { text = QStringLiteral("明天"); urgent = true; }
            else                { text = QStringLiteral("剩 %1 天").arg(days); }
            listItem->setData(RoleDueText, text);
            listItem->setData(RoleDueUrgent, urgent);
        }
        m_todoListWidget->addItem(listItem);
    }
    update();
}

void DesktopWidget::updateHeader()
{
    const QDate today = QDate::currentDate();
    const QString weekNames = QStringLiteral("一二三四五六日");
    m_dateLabel->setText(QStringLiteral("%1月%2日 周%3")
                         .arg(today.month()).arg(today.day())
                         .arg(weekNames.at(today.dayOfWeek() - 1)));

    const int n = m_displayItems.size();
    m_countLabel->setText(n > 0 ? QStringLiteral("还有 %1 件待办").arg(n)
                                : QStringLiteral("全部完成啦"));

    // 倒计时贴片：最近的未完成到期事项（m_displayItems 已按到期排序，取第一个有到期日的）
    m_dueUrgent = false;
    QString dueText;
    for (const TodoItem &item : m_displayItems) {
        if (!item.getDueDate().isValid()) continue;
        const qint64 days = today.daysTo(item.getDueDate());
        QString title = item.getTitle();
        if (title.length() > 6) title = title.left(6) + QStringLiteral("…");
        if (days < 0)       { dueText = QStringLiteral("「%1」已过期 %2 天").arg(title).arg(-days); m_dueUrgent = true; }
        else if (days == 0) { dueText = QStringLiteral("今天到期「%1」").arg(title); m_dueUrgent = true; }
        else if (days == 1) { dueText = QStringLiteral("明天到期「%1」").arg(title); m_dueUrgent = true; }
        else                { dueText = QStringLiteral("距「%1」还有 %2 天").arg(title).arg(days); }
        break;
    }
    m_dueLabel->setText(dueText);
    m_dueLabel->setVisible(!dueText.isEmpty());
    m_dueLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px; background: transparent;%2")
        .arg(m_dueUrgent ? accentColor().name() : kTextWarm.name(),
             m_dueUrgent ? QStringLiteral(" font-weight: 700;") : QString()));

    updateQuote();
}

void DesktopWidget::updateQuote()
{
    const int count = static_cast<int>(kDailyQuotes.size());
    const int idx = (QDate::currentDate().dayOfYear() + m_quoteOffset) % count;
    m_quoteLabel->setText(kDailyQuotes.at(idx));
}

void DesktopWidget::refreshDisplay()
{
    loadPendingItems();
    updateTodoList();
    updateHeader();
}

// ==========================================================
// 交互
// ==========================================================

void DesktopWidget::onAddTodoClicked()
{
    const QString title = m_addLineEdit->text().trimmed();
    if (!title.isEmpty()) {
        emit newTodoRequested(title);
        m_addLineEdit->clear();
    }
}

void DesktopWidget::onTodoItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    // 点击复选框区域：切换完成状态
    const QRect vr = m_todoListWidget->visualItemRect(item);
    const QPoint pos = m_todoListWidget->viewport()->mapFromGlobal(QCursor::pos());
    if (StickyNoteDelegate::checkRect(vr).contains(pos)) {
        emit todoItemToggled(item->data(RoleId).toString(), !item->data(RoleCompleted).toBool());
    }
}

void DesktopWidget::onRefreshTimer()
{
    updateHeader();
}

// ==========================================================
// 绘制
// ==========================================================

QRect DesktopWidget::noteRect() const
{
    return rect().adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
}

void DesktopWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    drawPaper(p);
    drawStarSticker(p);
    drawCharacter(p);
    drawProgressRing(p);
    drawTape(p);
    drawWeather(p);

    // 空状态提示
    if (m_displayItems.isEmpty()) {
        p.setFont(Theme::font(13));
        p.setPen(QColor(kTextSoft.red(), kTextSoft.green(), kTextSoft.blue(), 170));
        QRect area = noteRect().adjusted(0, 60, 0, -50);
        p.drawText(area, Qt::AlignHCenter | Qt::AlignTop,
                   QStringLiteral("暂无待办，喝杯茶吧~"));
    }
}

void DesktopWidget::drawPaper(QPainter &p)
{
    const QRect note = noteRect();

    // 柔和投影（多层外扩模拟模糊）
    for (int i = 7; i >= 1; --i) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(90, 70, 30, 5));
        p.drawRoundedRect(note.adjusted(-i, -i + 3, i, i + 3), 12 + i, 12 + i);
    }

    // 纸张渐变
    QLinearGradient grad(note.topLeft(), note.bottomLeft());
    grad.setColorAt(0, paperTop());
    grad.setColorAt(1, paperBottom());
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(note, 12, 12);

    // 纸张横线纹理（极淡）
    p.setPen(QPen(QColor(kTextSoft.red(), kTextSoft.green(), kTextSoft.blue(), 18), 1));
    p.setClipRect(note.adjusted(10, 0, -10, -8));
    for (int y = note.top() + 84; y < note.bottom() - 10; y += 30) {
        p.drawLine(note.left() + 12, y, note.right() - 12, y);
    }
    p.setClipping(false);
}

void DesktopWidget::drawTape(QPainter &p)
{
    const QRect note = noteRect();

    p.save();
    p.translate(note.center().x(), note.top());
    p.rotate(-2.5);

    QRectF tape(-50, -11, 100, 22);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 120));
    p.drawRoundedRect(tape, 3, 3);

    // 胶带斜纹
    p.setClipRect(tape);
    QColor stripe = accentColor();
    stripe.setAlpha(36);
    p.setPen(QPen(stripe, 4));
    for (qreal x = tape.left() - tape.height(); x < tape.right(); x += 12) {
        p.drawLine(QPointF(x, tape.bottom()), QPointF(x + tape.height(), tape.top()));
    }
    p.restore();
}

void DesktopWidget::drawStarSticker(QPainter &p)
{
    const QRect note = noteRect();
    const QPointF c(note.left() + 26, note.top() + 34);
    const double ro = 9.0, ri = 3.8;

    QPainterPath star;
    for (int i = 0; i < 10; ++i) {
        const double angle = -M_PI / 2 + i * M_PI / 5;
        const double r = (i % 2 == 0) ? ro : ri;
        const QPointF pt(c.x() + r * std::cos(angle), c.y() + r * std::sin(angle));
        if (i == 0) star.moveTo(pt); else star.lineTo(pt);
    }
    star.closeSubpath();

    p.save();
    p.translate(c);
    p.rotate(15);
    p.translate(-c);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xF6, 0xA5, 0xC0, 230));
    p.drawPath(star);
    p.restore();
}

void DesktopWidget::drawCharacter(QPainter &p)
{
    switch (m_character) {
    case CharRabbit: drawRabbit(p); break;
    case CharShiba:  drawShiba(p);  break;
    default:         drawCat(p);    break;
    }
}

void DesktopWidget::drawCat(QPainter &p)
{
    const QRect note = noteRect();
    const QPointF c(note.right() - 42, note.bottom() - 14);   // 猫头中心
    const double r = 18;

    const QColor face(0xFF, 0xFD, 0xF8);
    const QPen linePen(kLineBrown, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    // 耳朵（在头后面）
    p.setPen(linePen);
    p.setBrush(face);
    QPainterPath earL, earR;
    earL.moveTo(c.x() - 15, c.y() - 8);  earL.lineTo(c.x() - 18, c.y() - 25); earL.lineTo(c.x() - 4, c.y() - 15); earL.closeSubpath();
    earR.moveTo(c.x() + 15, c.y() - 8);  earR.lineTo(c.x() + 18, c.y() - 25); earR.lineTo(c.x() + 4, c.y() - 15); earR.closeSubpath();
    p.drawPath(earL);
    p.drawPath(earR);

    // 内耳
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xF8, 0xC8, 0xD0));
    QPainterPath innerL, innerR;
    innerL.moveTo(c.x() - 13.5, c.y() - 11); innerL.lineTo(c.x() - 15.5, c.y() - 20); innerL.lineTo(c.x() - 7, c.y() - 14); innerL.closeSubpath();
    innerR.moveTo(c.x() + 13.5, c.y() - 11); innerR.lineTo(c.x() + 15.5, c.y() - 20); innerR.lineTo(c.x() + 7, c.y() - 14); innerR.closeSubpath();
    p.drawPath(innerL);
    p.drawPath(innerR);

    // 头
    p.setPen(linePen);
    p.setBrush(face);
    p.drawEllipse(c, r, r);

    // 眼睛：平时圆点，眨眼时弯月
    p.setPen(linePen);
    if (m_blink) {
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(c.x() - 11, c.y() - 6, 8, 7), 200 * 16, 140 * 16);
        p.drawArc(QRectF(c.x() + 3,  c.y() - 6, 8, 7), 200 * 16, 140 * 16);
    } else {
        p.setBrush(kLineBrown);
        p.drawEllipse(QPointF(c.x() - 7, c.y() - 2), 2.0, 2.4);
        p.drawEllipse(QPointF(c.x() + 7, c.y() - 2), 2.0, 2.4);
    }

    // 嘴（小 w）
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(c.x() - 4.5, c.y() + 2, 4.5, 4), 200 * 16, 140 * 16);
    p.drawArc(QRectF(c.x(),      c.y() + 2, 4.5, 4), 200 * 16, 140 * 16);

    // 腮红
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xF8, 0xA5, 0xB8, 150));
    p.drawEllipse(QPointF(c.x() - 12.5, c.y() + 4), 3.2, 2.4);
    p.drawEllipse(QPointF(c.x() + 12.5, c.y() + 4), 3.2, 2.4);

    // 爪子搭在纸张下缘
    p.setPen(linePen);
    p.setBrush(face);
    p.drawEllipse(QPointF(c.x() - 11, note.bottom() - 2), 5.5, 4);
    p.drawEllipse(QPointF(c.x() + 11, note.bottom() - 2), 5.5, 4);
}

void DesktopWidget::drawRabbit(QPainter &p)
{
    const QRect note = noteRect();
    const QPointF c(note.right() - 42, note.bottom() - 14);   // 兔头中心
    const double r = 18;

    const QColor face(0xFF, 0xFD, 0xF8);
    const QColor pink(0xF8, 0xC8, 0xD0);
    const QPen linePen(kLineBrown, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    // 长耳朵（在头后面，微微外撇）
    auto drawEar = [&](qreal cx, qreal cy, qreal angle) {
        p.save();
        p.translate(cx, cy);
        p.rotate(angle);
        p.setPen(linePen);
        p.setBrush(face);
        p.drawEllipse(QRectF(-3.5, -24, 7, 24));        // 长 24 宽 7
        p.setPen(Qt::NoPen);
        p.setBrush(pink);
        p.drawEllipse(QRectF(-1.6, -20, 3.2, 16));      // 内耳
        p.restore();
    };
    drawEar(c.x() - 7, c.y() - 13, -10);
    drawEar(c.x() + 7, c.y() - 13, 10);

    // 头
    p.setPen(linePen);
    p.setBrush(face);
    p.drawEllipse(c, r, r);

    // 眼睛：平时圆点，眨眼时弯月
    p.setPen(linePen);
    if (m_blink) {
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(c.x() - 11, c.y() - 6, 8, 7), 200 * 16, 140 * 16);
        p.drawArc(QRectF(c.x() + 3,  c.y() - 6, 8, 7), 200 * 16, 140 * 16);
    } else {
        p.setBrush(kLineBrown);
        p.drawEllipse(QPointF(c.x() - 7, c.y() - 2), 2.0, 2.4);
        p.drawEllipse(QPointF(c.x() + 7, c.y() - 2), 2.0, 2.4);
    }

    // 小粉鼻
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xF8, 0xA5, 0xB8));
    p.drawEllipse(QPointF(c.x(), c.y() + 1.5), 1.8, 1.4);

    // 嘴（小 w）
    p.setPen(linePen);
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(c.x() - 4.5, c.y() + 3, 4.5, 4), 200 * 16, 140 * 16);
    p.drawArc(QRectF(c.x(),      c.y() + 3, 4.5, 4), 200 * 16, 140 * 16);

    // 腮红
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xF8, 0xA5, 0xB8, 150));
    p.drawEllipse(QPointF(c.x() - 12.5, c.y() + 5), 3.2, 2.4);
    p.drawEllipse(QPointF(c.x() + 12.5, c.y() + 5), 3.2, 2.4);

    // 爪子搭在纸张下缘
    p.setPen(linePen);
    p.setBrush(face);
    p.drawEllipse(QPointF(c.x() - 11, note.bottom() - 2), 5.5, 4);
    p.drawEllipse(QPointF(c.x() + 11, note.bottom() - 2), 5.5, 4);
}

void DesktopWidget::drawShiba(QPainter &p)
{
    const QRect note = noteRect();
    const QPointF c(note.right() - 42, note.bottom() - 14);   // 柴犬头中心
    const double r = 18;

    const QColor fur(0xF6, 0xC1, 0x77);      // 暖橙
    const QColor cream(0xFF, 0xFD, 0xF8);
    const QPen linePen(kLineBrown, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    // 三角耳（在头后面）
    p.setPen(linePen);
    p.setBrush(fur);
    QPainterPath earL, earR;
    earL.moveTo(c.x() - 15, c.y() - 8);  earL.lineTo(c.x() - 17, c.y() - 24); earL.lineTo(c.x() - 4, c.y() - 15); earL.closeSubpath();
    earR.moveTo(c.x() + 15, c.y() - 8);  earR.lineTo(c.x() + 17, c.y() - 24); earR.lineTo(c.x() + 4, c.y() - 15); earR.closeSubpath();
    p.drawPath(earL);
    p.drawPath(earR);

    // 内耳
    p.setPen(Qt::NoPen);
    p.setBrush(cream);
    QPainterPath innerL, innerR;
    innerL.moveTo(c.x() - 13, c.y() - 11); innerL.lineTo(c.x() - 14.5, c.y() - 19); innerL.lineTo(c.x() - 7, c.y() - 14); innerL.closeSubpath();
    innerR.moveTo(c.x() + 13, c.y() - 11); innerR.lineTo(c.x() + 14.5, c.y() - 19); innerR.lineTo(c.x() + 7, c.y() - 14); innerR.closeSubpath();
    p.drawPath(innerL);
    p.drawPath(innerR);

    // 头
    p.setPen(linePen);
    p.setBrush(fur);
    p.drawEllipse(c, r, r);

    // 白色口鼻区域（脸下半部分）
    p.setPen(Qt::NoPen);
    p.setBrush(cream);
    p.drawEllipse(QPointF(c.x(), c.y() + 7), 10.5, 8);

    // 眼睛：平时圆点，眨眼时弯月
    p.setPen(linePen);
    if (m_blink) {
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(c.x() - 11, c.y() - 6, 8, 7), 200 * 16, 140 * 16);
        p.drawArc(QRectF(c.x() + 3,  c.y() - 6, 8, 7), 200 * 16, 140 * 16);
    } else {
        p.setBrush(kLineBrown);
        p.drawEllipse(QPointF(c.x() - 7, c.y() - 2), 2.0, 2.4);
        p.drawEllipse(QPointF(c.x() + 7, c.y() - 2), 2.0, 2.4);
    }

    // 小黑鼻
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x40, 0x2E, 0x28));
    p.drawEllipse(QPointF(c.x(), c.y() + 3), 2.2, 1.8);

    // 嘴（小 w）
    p.setPen(linePen);
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(c.x() - 4.5, c.y() + 4.5, 4.5, 4), 200 * 16, 140 * 16);
    p.drawArc(QRectF(c.x(),      c.y() + 4.5, 4.5, 4), 200 * 16, 140 * 16);

    // 腮红
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xF8, 0xA5, 0xB8, 150));
    p.drawEllipse(QPointF(c.x() - 12.5, c.y() + 3), 3.2, 2.4);
    p.drawEllipse(QPointF(c.x() + 12.5, c.y() + 3), 3.2, 2.4);

    // 白色爪子搭在纸张下缘
    p.setPen(linePen);
    p.setBrush(cream);
    p.drawEllipse(QPointF(c.x() - 11, note.bottom() - 2), 5.5, 4);
    p.drawEllipse(QPointF(c.x() + 11, note.bottom() - 2), 5.5, 4);
}

void DesktopWidget::drawProgressRing(QPainter &p)
{
    // 进度 = 所有文件夹中已完成事项数 / 总事项数
    int total = 0, done = 0;
    for (const TodoFolder &folder : m_folders) {
        for (const TodoItem &item : folder.getItems()) {
            ++total;
            if (item.isCompleted()) ++done;
        }
    }
    if (total == 0) return;

    const QRect note = noteRect();
    const QPointF c(note.left() + 14, note.bottom() - 14);   // 圆环中心（左下角，与猫咪呼应）
    const qreal outer = 30.0;                                // 外径
    const qreal penW = 4.0;
    const qreal r = (outer - penW) / 2.0;
    const qreal ratio = qreal(done) / qreal(total);

    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    // 底环
    QColor base = kLineBrown;
    base.setAlphaF(0.2);
    p.setPen(QPen(base, penW));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, r, r);

    // 进度弧（12 点方向顺时针）
    p.setPen(QPen(accentColor(), penW, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(QRectF(c.x() - r, c.y() - r, r * 2, r * 2), 90 * 16, -qRound(ratio * 360 * 16));

    // 中心百分比
    QFont f(QStringLiteral("Microsoft YaHei UI"), -1);
    f.setPixelSize(10);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    p.setPen(kTextWarm);
    p.drawText(QRectF(c.x() - outer / 2, c.y() - outer / 2, outer, outer),
               Qt::AlignCenter, QStringLiteral("%1%").arg(qRound(ratio * 100)));

    // 全部完成：在猫咪周围撒彩带庆祝
    if (done == total) {
        auto *rng = QRandomGenerator::global();
        const QPointF catC(note.right() - 42, note.bottom() - 24);
        const QList<QColor> palette = {
            accentColor(),
            QColor(0xF6, 0xA5, 0xC0),   // 粉
            QColor(0x42, 0xA5, 0xF5),   // 蓝
            QColor(0xFF, 0xD5, 0x4F)    // 暖黄
        };
        const int pieces = 6 + rng->bounded(3);   // 6-8 片
        for (int i = 0; i < pieces; ++i) {
            const double ang = rng->generateDouble() * 2 * M_PI;
            const double dist = 20 + rng->generateDouble() * 14;
            p.save();
            p.translate(catC.x() + dist * std::cos(ang), catC.y() + dist * std::sin(ang));
            p.rotate(rng->generateDouble() * 360.0);
            p.setPen(Qt::NoPen);
            p.setBrush(palette.at(rng->bounded(static_cast<int>(palette.size()))));
            if (rng->bounded(2) == 0) {
                p.drawRect(QRectF(-2.5, -2.5, 5, 5));   // 小方块
            } else {
                QPainterPath tri;                        // 小三角
                tri.moveTo(0, -3.4);
                tri.lineTo(3.0, 2.4);
                tri.lineTo(-3.0, 2.4);
                tri.closeSubpath();
                p.drawPath(tri);
            }
            p.restore();
        }
    }

    p.restore();
}

void DesktopWidget::scheduleBlink()
{
    const int interval = 2400 + QRandomGenerator::global()->bounded(3200);
    QTimer::singleShot(interval, this, [this]() {
        m_blink = true;
        update();
        QTimer::singleShot(180, this, [this]() {
            m_blink = false;
            update();
            scheduleBlink();
        });
    });
}

// ==========================================================
// 天气贴片
// ==========================================================

QRect DesktopWidget::weatherRect() const
{
    if (!m_pinButton) return QRect();
    const QRect pin = m_pinButton->geometry();
    return QRect(pin.left() - 6 - 98, pin.top() + 1, 98, 24);
}

QString DesktopWidget::weatherText() const
{
    if (m_weatherLoading) return QStringLiteral("%1 …").arg(m_weatherCity);
    if (m_weatherCode < 0) return QStringLiteral("%1 点我").arg(m_weatherCity);
    return QStringLiteral("%1 %2°").arg(m_weatherCity).arg(m_weatherTemp);
}

void DesktopWidget::drawWeather(QPainter &p)
{
    const QRect r = weatherRect();
    if (r.isNull()) return;

    p.save();

    // 小贴纸底（半透明白纸）
    p.setPen(QPen(QColor(accentColor().red(), accentColor().green(), accentColor().blue(), 70), 1));
    p.setBrush(QColor(255, 255, 255, 150));
    p.drawRoundedRect(r, 12, 12);

    // 手绘天气图标
    const QRectF iconBox(r.left() + 7, r.center().y() - 7.5, 15, 15);
    const QPen linePen(kLineBrown, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    const int kind = (m_weatherCode < 0) ? WSun : weatherKind(m_weatherCode);

    auto drawSun = [&](const QRectF &b) {
        p.setPen(QPen(accentColor(), 1.6, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(QColor(accentColor().red(), accentColor().green(), accentColor().blue(), 90));
        p.drawEllipse(b.adjusted(3, 3, -3, -3));
        for (int i = 0; i < 8; ++i) {
            const double a = i * M_PI / 4;
            p.drawLine(QPointF(b.center().x() + (b.width() / 2 - 1.5) * std::cos(a),
                               b.center().y() + (b.height() / 2 - 1.5) * std::sin(a)),
                       QPointF(b.center().x() + (b.width() / 2 + 1.5) * std::cos(a),
                               b.center().y() + (b.height() / 2 + 1.5) * std::sin(a)));
        }
    };
    auto drawCloud = [&](const QRectF &b) {
        p.setPen(linePen);
        p.setBrush(QColor(0xFF, 0xFD, 0xF8));
        p.drawPath(cloudPath(b));
    };

    if (m_weatherLoading) {
        // 加载中：旋转小圆弧
        p.setPen(QPen(kTextSoft, 1.6, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(iconBox, 45 * 16, 240 * 16);
    } else if (kind == WSun) {
        drawSun(iconBox);
    } else if (kind == WPartly) {
        drawSun(QRectF(iconBox.left(), iconBox.top(), iconBox.width() * 0.75, iconBox.height() * 0.75));
        drawCloud(QRectF(iconBox.left() + 5, iconBox.top() + 6, iconBox.width() - 5, iconBox.height() - 6));
    } else if (kind == WCloud) {
        drawCloud(iconBox);
    } else if (kind == WRain || kind == WThunder) {
        drawCloud(QRectF(iconBox.left(), iconBox.top(), iconBox.width(), iconBox.height() - 5));
        if (kind == WThunder) {
            QPainterPath bolt;
            bolt.moveTo(iconBox.center().x() + 1, iconBox.bottom() - 6);
            bolt.lineTo(iconBox.center().x() - 2.5, iconBox.bottom() - 2);
            bolt.lineTo(iconBox.center().x() + 0.5, iconBox.bottom() - 2);
            bolt.lineTo(iconBox.center().x() - 2, iconBox.bottom() + 2);
            p.setPen(Qt::NoPen);
            p.setBrush(accentColor());
            p.drawPath(bolt);
        } else {
            p.setPen(QPen(QColor(0x42, 0xA5, 0xF5), 1.5, Qt::SolidLine, Qt::RoundCap));
            for (int i = 0; i < 3; ++i) {
                const qreal x = iconBox.left() + 4 + i * 4.5;
                p.drawLine(QPointF(x, iconBox.bottom() - 5), QPointF(x - 1.5, iconBox.bottom() - 1));
            }
        }
    } else if (kind == WSnow) {
        drawCloud(QRectF(iconBox.left(), iconBox.top(), iconBox.width(), iconBox.height() - 5));
        p.setPen(QPen(QColor(0x64, 0xB5, 0xF6), 1.3, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 2; ++i) {
            const QPointF c(iconBox.left() + 5 + i * 7, iconBox.bottom() - 2);
            p.drawLine(c + QPointF(-2, 0), c + QPointF(2, 0));
            p.drawLine(c + QPointF(0, -2), c + QPointF(0, 2));
        }
    } else { // WFog
        drawCloud(QRectF(iconBox.left(), iconBox.top(), iconBox.width(), iconBox.height() - 6));
        p.setPen(QPen(kTextSoft, 1.3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(iconBox.left() + 2, iconBox.bottom() - 3), QPointF(iconBox.right() - 4, iconBox.bottom() - 3));
        p.drawLine(QPointF(iconBox.left() + 5, iconBox.bottom()), QPointF(iconBox.right() - 1, iconBox.bottom()));
    }

    // 文字（城市 + 温度）
    QFont f(QStringLiteral("Microsoft YaHei UI"), -1);
    f.setPixelSize(12);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    p.setPen(kTextWarm);
    QFontMetrics fm(f);
    const int textX = r.left() + 26;
    p.drawText(QRect(textX, r.top(), r.right() - 6 - textX, r.height()),
               Qt::AlignLeft | Qt::AlignVCenter,
               fm.elidedText(weatherText(), Qt::ElideRight, r.right() - 6 - textX));

    p.restore();
}

void DesktopWidget::fetchWeather()
{
    if (m_weatherLoading || !m_netManager) return;
    m_weatherLoading = true;
    update(weatherRect());

    const QString url = QStringLiteral("https://wttr.in/%1?format=j1&lang=zh")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_weatherCity)));
    QNetworkReply *reply = m_netManager->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onWeatherReply(reply); });
}

void DesktopWidget::onWeatherReply(QNetworkReply *reply)
{
    reply->deleteLater();
    m_weatherLoading = false;

    if (reply->error() == QNetworkReply::NoError) {
        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonArray cur = root[QStringLiteral("current_condition")].toArray();
        if (!cur.isEmpty()) {
            const QJsonObject c = cur.first().toObject();
            m_weatherTemp = qRound(c[QStringLiteral("temp_C")].toString().toDouble());
            m_weatherCode = c[QStringLiteral("weatherCode")].toString().toInt();
            const QJsonArray zh = c[QStringLiteral("lang_zh")].toArray();
            m_weatherDesc = zh.isEmpty()
                ? c[QStringLiteral("weatherDesc")].toArray().first().toObject()
                      [QStringLiteral("value")].toString()
                : zh.first().toObject()[QStringLiteral("value")].toString();
        }
    } else {
        m_weatherCode = -1;   // 获取失败，显示"点我"提示重试
    }
    update(weatherRect());
}

void DesktopWidget::promptSetCity()
{
    bool ok = false;
    const QString city = QInputDialog::getText(this, QStringLiteral("设置城市"),
                                               QStringLiteral("输入城市名（如：上海）："),
                                               QLineEdit::Normal, m_weatherCity, &ok);
    const QString trimmed = city.trimmed();
    if (ok && !trimmed.isEmpty() && trimmed != m_weatherCity) {
        m_weatherCity = trimmed;
        saveAppearance();
        fetchWeather();
    }
}

// ==========================================================
// 拖拽 / 缩放
// ==========================================================

void DesktopWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPoint pos = event->pos();
        if (weatherRect().contains(pos)) {
            fetchWeather();   // 点击天气贴片：刷新
            event->accept();
            return;
        }
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
    if (m_resizing) {
        const QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        int newWidth = m_resizeStartSize.width();
        int newHeight = m_resizeStartSize.height();
        int newX = m_resizeStartWindowPos.x();
        int newY = m_resizeStartWindowPos.y();

        if (m_resizeEdge & 0x01) newWidth = qMax(minimumWidth(), m_resizeStartSize.width() + delta.x());
        if (m_resizeEdge & 0x02) {
            newWidth = qMax(minimumWidth(), m_resizeStartSize.width() - delta.x());
            newX = m_resizeStartWindowPos.x() + (m_resizeStartSize.width() - newWidth);
        }
        if (m_resizeEdge & 0x04) newHeight = qMax(minimumHeight(), m_resizeStartSize.height() + delta.y());
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
        updateCursor(event->pos());
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

void DesktopWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 双击空白：聚焦快速添加（小黄条式"点一下就写"）
    if (event->button() == Qt::LeftButton) {
        m_addLineEdit->setFocus();
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool DesktopWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_todoListWidget->viewport() && event->type() == QEvent::MouseMove) {
        auto *mouseEvent = static_cast<QMouseEvent*>(event);
        updateCursor(mapFromGlobal(mouseEvent->globalPosition().toPoint()));
    }
    // 每日一句：点击切换到下一条
    if (obj == m_quoteLabel && event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_quoteOffset = (m_quoteOffset + 1) % static_cast<int>(kDailyQuotes.size());
            updateQuote();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void DesktopWidget::updateCursor(const QPoint &pos)
{
    if (m_dragging || m_resizing) return;

    if (weatherRect().contains(pos)) {
        setCursor(Qt::PointingHandCursor);
        return;
    }

    int edge = 0;
    const int margin = 10;
    if (pos.x() >= width() - margin)  edge |= 0x01;
    if (pos.x() <= margin)            edge |= 0x02;
    if (pos.y() >= height() - margin) edge |= 0x04;
    if (pos.y() <= margin)            edge |= 0x08;

    Qt::CursorShape cursor = Qt::ArrowCursor;
    if (edge == 0x01 || edge == 0x02)      cursor = Qt::SizeHorCursor;
    else if (edge == 0x04 || edge == 0x08) cursor = Qt::SizeVerCursor;
    else if (edge == 0x05 || edge == 0x0A) cursor = Qt::SizeFDiagCursor;
    else if (edge == 0x06 || edge == 0x09) cursor = Qt::SizeBDiagCursor;
    setCursor(cursor);
}

bool DesktopWidget::isOnResizeArea(const QPoint &pos)
{
    const int margin = 10;
    m_resizeEdge = 0;
    if (pos.x() >= width() - margin)  m_resizeEdge |= 0x01;
    if (pos.x() <= margin)            m_resizeEdge |= 0x02;
    if (pos.y() >= height() - margin) m_resizeEdge |= 0x04;
    if (pos.y() <= margin)            m_resizeEdge |= 0x08;
    return m_resizeEdge != 0;
}

// ==========================================================
// 右键菜单
// ==========================================================

void DesktopWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(
        "QMenu { background-color: #fffdf6; border: 1px solid #e8dcc8; border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 7px 24px; border-radius: 4px; color: #5d4037; }"
        "QMenu::item:selected { background-color: %1; color: #5d4037; }"
        "QMenu::item:checked { font-weight: 600; }")
        .arg(QColor(accentColor().red(), accentColor().green(), accentColor().blue(), 50).name(QColor::HexArgb)));

    // 命中待办项：完成/编辑/删除
    QListWidgetItem *hit = m_todoListWidget->itemAt(m_todoListWidget->mapFromParent(event->pos()));
    if (hit) {
        const QString itemId = hit->data(RoleId).toString();
        const bool completed = hit->data(RoleCompleted).toBool();

        menu.addAction(completed ? QStringLiteral("标记为未完成") : QStringLiteral("标记为已完成"),
                       this, [this, itemId, completed]() { emit todoItemToggled(itemId, !completed); });
        menu.addAction(QStringLiteral("编辑"), this, [this, itemId]() { emit editTodoRequested(itemId); });
        menu.addAction(QStringLiteral("删除"), this, [this, itemId]() { emit deleteTodoRequested(itemId); });
        menu.addSeparator();
    }

    // 纸张颜色
    QMenu *themeMenu = menu.addMenu(QStringLiteral("纸张颜色"));
    const QStringList names = {QStringLiteral("柠檬黄"), QStringLiteral("樱花粉"),
                               QStringLiteral("薄荷绿"), QStringLiteral("天空蓝"), QStringLiteral("奶油白")};
    for (int i = 0; i < names.size(); ++i) {
        QAction *a = themeMenu->addAction(names[i], this, [this, i]() { setPaperTheme(static_cast<PaperTheme>(i)); });
        a->setCheckable(true);
        a->setChecked(static_cast<int>(m_theme) == i);
    }

    // 小伙伴
    QMenu *charMenu = menu.addMenu(QStringLiteral("小伙伴"));
    const QStringList charNames = {QStringLiteral("猫咪"), QStringLiteral("兔子"), QStringLiteral("柴犬")};
    for (int i = 0; i < charNames.size(); ++i) {
        QAction *a = charMenu->addAction(charNames[i], this, [this, i]() { setCharacter(static_cast<Character>(i)); });
        a->setCheckable(true);
        a->setChecked(static_cast<int>(m_character) == i);
    }

    // 透明度
    QMenu *opacityMenu = menu.addMenu(QStringLiteral("透明度"));
    const QList<QPair<QString, qreal>> levels = {
        {QStringLiteral("不透明"), 1.0}, {QStringLiteral("90%"), 0.9}, {QStringLiteral("75%"), 0.75}
    };
    for (const auto &lv : levels) {
        QAction *a = opacityMenu->addAction(lv.first, this, [this, lv]() { setNoteOpacity(lv.second); });
        a->setCheckable(true);
        a->setChecked(qAbs(windowOpacity() - lv.second) < 0.01);
    }

    QAction *pinAction = menu.addAction(QStringLiteral("置顶显示"), this, [this]() { setAlwaysOnTop(!m_alwaysOnTop); });
    pinAction->setCheckable(true);
    pinAction->setChecked(m_alwaysOnTop);

    // 天气
    QMenu *weatherMenu = menu.addMenu(QStringLiteral("天气"));
    if (!m_weatherDesc.isEmpty() && m_weatherCode >= 0) {
        QAction *info = weatherMenu->addAction(QStringLiteral("%1 · %2 · %3°C")
                                               .arg(m_weatherCity, m_weatherDesc)
                                               .arg(m_weatherTemp));
        info->setEnabled(false);
        weatherMenu->addSeparator();
    }
    weatherMenu->addAction(QStringLiteral("设置城市…"), this, &DesktopWidget::promptSetCity);
    weatherMenu->addAction(QStringLiteral("刷新天气"), this, &DesktopWidget::fetchWeather);

    menu.addSeparator();
    menu.addAction(QStringLiteral("刷新"), this, &DesktopWidget::refreshDisplay);
    menu.addAction(QStringLiteral("打开主窗口"), this, [this]() { emit showMainWindowRequested(); });
    menu.addAction(QStringLiteral("隐藏便签"), this, &DesktopWidget::hide);

    menu.exec(event->globalPos());
}

void DesktopWidget::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

// ==========================================================
// 设置持久化
// ==========================================================

void DesktopWidget::saveWindowPosition()
{
    QSettings settings;
    settings.setValue(QStringLiteral("DesktopWidget/position"), pos());
}

void DesktopWidget::loadWindowPosition()
{
    QSettings settings;
    QPoint savedPos = settings.value(QStringLiteral("DesktopWidget/position"), QPoint(100, 100)).toPoint();

    QScreen *screen = QApplication::primaryScreen();
    const QRect available = screen->availableGeometry();

    if (savedPos.x() < 0 || savedPos.x() > available.width() - width()) {
        savedPos.setX(available.width() - width() - 50);
    }
    if (savedPos.y() < 0 || savedPos.y() > available.height() - height()) {
        savedPos.setY(100);
    }
    move(savedPos);
}

void DesktopWidget::saveWindowSize()
{
    QSettings settings;
    settings.setValue(QStringLiteral("DesktopWidget/size"), size());
}

void DesktopWidget::loadWindowSize()
{
    QSettings settings;
    QSize savedSize = settings.value(QStringLiteral("DesktopWidget/size"), QSize(300, 400)).toSize();
    savedSize = savedSize.expandedTo(minimumSize());

    QScreen *screen = QApplication::primaryScreen();
    const QRect available = screen->availableGeometry();
    savedSize.setWidth(qMin(savedSize.width(), available.width() - 50));
    savedSize.setHeight(qMin(savedSize.height(), available.height() - 50));
    resize(savedSize);
}

void DesktopWidget::saveAppearance()
{
    QSettings settings;
    settings.setValue(QStringLiteral("DesktopWidget/theme"), static_cast<int>(m_theme));
    settings.setValue(QStringLiteral("DesktopWidget/character"), static_cast<int>(m_character));
    settings.setValue(QStringLiteral("DesktopWidget/opacity"), windowOpacity());
    settings.setValue(QStringLiteral("DesktopWidget/onTop"), m_alwaysOnTop);
    settings.setValue(QStringLiteral("DesktopWidget/weatherCity"), m_weatherCity);
}

void DesktopWidget::loadAppearance()
{
    QSettings settings;
    m_theme = static_cast<PaperTheme>(qBound(0, settings.value(QStringLiteral("DesktopWidget/theme"), 0).toInt(), 4));
    m_character = static_cast<Character>(qBound(0, settings.value(QStringLiteral("DesktopWidget/character"), 0).toInt(), 2));
    const qreal opacity = qBound(0.3, settings.value(QStringLiteral("DesktopWidget/opacity"), 1.0).toReal(), 1.0);
    m_alwaysOnTop = settings.value(QStringLiteral("DesktopWidget/onTop"), true).toBool();
    m_weatherCity = settings.value(QStringLiteral("DesktopWidget/weatherCity"), QStringLiteral("北京")).toString();

    setWindowOpacity(opacity);
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool;
    if (m_alwaysOnTop) flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
}
