#include "desktopwidget.h"
#include "../icons.h"
#include "../theme.h"
#include "../components/messageutils.h"

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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QRandomGenerator>
#include <QVariantAnimation>
#include <QEasingCurve>
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

constexpr int kShadowMargin = 14;   // 玻璃纸外圈阴影留白

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

// ---- 霓虹进度条：玻璃轨道 + 青紫渐变填充 + 发光（带平滑动画） ----
class NeonProgressBar : public QWidget
{
public:
    explicit NeonProgressBar(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedHeight(10);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_anim = new QVariantAnimation(this);
        m_anim->setDuration(600);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
            m_ratio = v.toReal();
            update();
        });
    }

    void setRatio(qreal ratio)
    {
        ratio = qBound<qreal>(0.0, ratio, 1.0);
        m_anim->stop();
        m_anim->setStartValue(m_ratio);
        m_anim->setEndValue(ratio);
        m_anim->start();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRectF track(0, (height() - 5) / 2.0, width(), 5);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::glassBgStrong());
        p.drawRoundedRect(track, 2.5, 2.5);

        if (m_ratio > 0.001) {
            const qreal w = qMax<qreal>(5.0, track.width() * m_ratio);
            const QRectF fill(track.left(), track.top(), w, track.height());

            // 发光底层
            p.setBrush(Theme::withAlpha(Theme::primary(), 40));
            p.drawRoundedRect(fill.adjusted(0, -2, 0, 2), 4, 4);

            QLinearGradient grad(track.topLeft(), track.topRight());
            grad.setColorAt(0, Theme::primary());
            grad.setColorAt(1, Theme::accent());
            p.setBrush(QBrush(grad));
            p.drawRoundedRect(fill, 2.5, 2.5);
        }
    }

private:
    qreal m_ratio = 0.0;
    QVariantAnimation *m_anim = nullptr;
};

// ---- 便利贴列表代理：霓虹复选框 + 标题 + 倒数日贴片（暗色玻璃风） ----
class StickyNoteDelegate : public QStyledItemDelegate
{
public:
    explicit StickyNoteDelegate(QListWidget *view) : QStyledItemDelegate(view) {}

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

        // 悬停：玻璃光晕
        if (hovered && !completed) {
            p->setPen(Qt::NoPen);
            p->setBrush(Theme::hoverGlow());
            p->drawRoundedRect(option.rect.adjusted(2, 2, -2, -2), 8, 8);
        }

        int x = option.rect.left() + 4;
        const int cy = option.rect.center().y();

        // 霓虹圆圈复选框
        QRect circle(x + 2, cy - 8, 16, 16);
        if (completed) {
            QLinearGradient grad(circle.topLeft(), circle.bottomRight());
            grad.setColorAt(0, Theme::primary());
            grad.setColorAt(1, Theme::accent());
            p->setPen(Qt::NoPen);
            p->setBrush(QBrush(grad));
            p->drawEllipse(circle);
            QPainterPath check;
            check.moveTo(circle.left() + 4.5, circle.center().y());
            check.lineTo(circle.center().x() - 0.5, circle.bottom() - 4.5);
            check.lineTo(circle.right() - 4, circle.top() + 5);
            // 勾选线：深底用浅色、浅底用白色，保证在渐变填充上可读
            p->setPen(QPen(Theme::isDark() ? QColor(0x0B, 0x0E, 0x1A) : QColor(255, 255, 255),
                           2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p->setBrush(Qt::NoBrush);
            p->drawPath(check);
        } else {
            p->setPen(QPen(hovered ? Theme::primary() : Theme::withAlpha(Theme::textMuted(), 160), 1.8));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(circle);
        }
        x += 26;

        // 优先级点
        if (priority > 0 && !completed) {
            p->setPen(Qt::NoPen);
            p->setBrush(priority == 2 ? Theme::danger() : Theme::warning());
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

        // 倒数日（右侧贴片）
        int titleRight = option.rect.right() - 8;
        if (!due.isEmpty()) {
            QFont dueFont(QStringLiteral("Microsoft YaHei UI"), -1);
            dueFont.setPixelSize(11);
            QFontMetrics dfm(dueFont);
            int dw = dfm.horizontalAdvance(due) + 12;
            QRect dueRect(titleRight - dw, cy - 9, dw, 18);
            p->setPen(Qt::NoPen);
            p->setBrush(dueUrgent ? Theme::withAlpha(Theme::danger(), 40)
                                  : Theme::glassBgStrong());
            p->drawRoundedRect(dueRect, 9, 9);
            p->setFont(dueFont);
            p->setPen(dueUrgent ? Theme::danger() : Theme::textSecondary());
            p->drawText(dueRect, Qt::AlignCenter, due);
            titleRight = dueRect.left() - 6;
        }

        // 标题
        QFont titleFont(QStringLiteral("Microsoft YaHei UI"), -1);
        titleFont.setPixelSize(14);
        titleFont.setStrikeOut(completed);
        p->setFont(titleFont);
        p->setPen(completed ? Theme::textMuted() : Theme::textPrimary());
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

    // 内部粒子动效
    m_clock.start();
    initNoteParticles();
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(50);   // 20fps，便签足够流畅且省电
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        if (!isVisible()) {
            m_lastTick = m_clock.elapsed();
            return;
        }
        const qint64 now = m_clock.elapsed();
        const qreal dt = qMin<qreal>((now - m_lastTick) / 1000.0, 0.1);
        m_lastTick = now;
        advanceNoteParticles(dt);
        update();
    });
    m_animTimer->start();

    // 天气：立即获取 + 每 30 分钟自动刷新
    m_netManager = new QNetworkAccessManager(this);
    m_weatherTimer = new QTimer(this);
    m_weatherTimer->setInterval(30 * 60 * 1000);
    connect(m_weatherTimer, &QTimer::timeout, this, &DesktopWidget::fetchWeather);
    m_weatherTimer->start();
    fetchWeather();

    updateHeader();

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
    // 玻璃纸外侧留出阴影空间
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(kShadowMargin + 10, kShadowMargin + 14,
                                   kShadowMargin + 10, kShadowMargin + 10);
    mainLayout->setSpacing(4);

    // 头部：日期 + 天气贴片（绘制） + 置顶按钮
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

    // 霓虹进度条
    m_progressBar = new NeonProgressBar(this);
    mainLayout->addSpacing(4);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addSpacing(4);

    // 待办列表
    m_todoListWidget = new QListWidget(this);
    m_todoListWidget->setItemDelegate(new StickyNoteDelegate(m_todoListWidget));
    m_todoListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_todoListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_todoListWidget->setFrameShape(QFrame::NoFrame);
    mainLayout->addWidget(m_todoListWidget, 1);

    // 快速添加
    m_addLineEdit = new QLineEdit(this);
    m_addLineEdit->setPlaceholderText(QStringLiteral("+ 添加待办，回车保存"));
    mainLayout->addWidget(m_addLineEdit);
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

void DesktopWidget::applyTheme()
{
    m_dateLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 15px; font-weight: 700; background: transparent;")
        .arg(Theme::textPrimary().name()));
    m_countLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 11px; background: transparent;")
        .arg(Theme::textSecondary().name()));
    m_quoteLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px; font-style: italic; background: transparent;")
        .arg(Theme::textMuted().name()));

    m_todoListWidget->setStyleSheet(QStringLiteral(
        "QListWidget { border: none; background: transparent; }"
        "QScrollBar:vertical { background: transparent; width: 6px; margin: 2px; }"
        "QScrollBar::handle:vertical { background: %1; border-radius: 3px; min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: %2; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
        .arg(Theme::withAlpha(Theme::textMuted(), 90).name(QColor::HexArgb),
             Theme::withAlpha(Theme::primary(), 140).name(QColor::HexArgb)));

    m_addLineEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { border: none; border-bottom: 1px solid %1; background: transparent;"
        "            color: %2; font-size: 13px; padding: 6px 2px; }"
        "QLineEdit:focus { border-bottom: 1px solid %3; }")
        .arg(Theme::glassBorder().name(QColor::HexArgb),
             Theme::textPrimary().name(),
             Theme::primary().name()));

    m_pinButton->setStyleSheet(QStringLiteral(
        "QPushButton { border: none; background: transparent; border-radius: 13px; }"
        "QPushButton:hover { background: %1; }")
        .arg(Theme::hoverGlow().name(QColor::HexArgb)));
    m_pinButton->setIcon(Icons::icon(Icons::Pin, 14,
                                     m_alwaysOnTop ? Theme::primary()
                                                   : Theme::textMuted()));

    m_todoListWidget->viewport()->update();
    updateHeader();
    update();
}

void DesktopWidget::refreshTheme()
{
    applyTheme();
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

        // 倒数日：只提示未过期的（过期的不打扰）
        if (item.getDueDate().isValid()) {
            const qint64 days = today.daysTo(item.getDueDate());
            if (days >= 0) {
                QString text;
                bool urgent = false;
                if (days == 0)      { text = QStringLiteral("今天"); urgent = true; }
                else if (days == 1) { text = QStringLiteral("明天"); urgent = true; }
                else                { text = QStringLiteral("剩 %1 天").arg(days); }
                listItem->setData(RoleDueText, text);
                listItem->setData(RoleDueUrgent, urgent);
            }
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

    // 进度条：已完成 / 总数
    int total = 0, done = 0;
    for (const TodoFolder &folder : m_folders) {
        total += folder.getItemCount();
        done += folder.getCompletedCount();
    }
    static_cast<NeonProgressBar*>(m_progressBar)
        ->setRatio(total > 0 ? qreal(done) / qreal(total) : 0.0);

    // 倒计时贴片：最近的未完成到期事项（只提示未过期的）
    bool dueUrgent = false;
    QString dueText;
    for (const TodoItem &item : m_displayItems) {
        if (!item.getDueDate().isValid()) continue;
        const qint64 days = today.daysTo(item.getDueDate());
        if (days < 0) continue;   // 过期的不打扰
        QString title = item.getTitle();
        if (title.length() > 6) title = title.left(6) + QStringLiteral("…");
        if (days == 0)      { dueText = QStringLiteral("今天到期「%1」").arg(title); dueUrgent = true; }
        else if (days == 1) { dueText = QStringLiteral("明天到期「%1」").arg(title); dueUrgent = true; }
        else                { dueText = QStringLiteral("距「%1」还有 %2 天").arg(title).arg(days); }
        break;
    }
    m_dueLabel->setText(dueText);
    m_dueLabel->setVisible(!dueText.isEmpty());
    m_dueLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px; background: transparent;%2")
        .arg(dueUrgent ? Theme::danger().name() : Theme::textSecondary().name(),
             dueUrgent ? QStringLiteral(" font-weight: 700;") : QString()));

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

    drawNote(p);
    drawWeather(p);

    // 空状态提示
    if (m_displayItems.isEmpty()) {
        p.setFont(Theme::font(13));
        p.setPen(Theme::withAlpha(Theme::textMuted(), 190));
        QRect area = noteRect().adjusted(0, 90, 0, -60);
        p.drawText(area, Qt::AlignHCenter | Qt::AlignTop,
                   QStringLiteral("暂无待办，喝杯茶吧~"));
    }
}

void DesktopWidget::initNoteParticles()
{
    m_particles.clear();
    const QRect note = noteRect();
    m_particleArea = note.size();

    auto *rng = QRandomGenerator::global();
    const int count = qBound(12, note.width() * note.height() / 11000, 20);
    for (int i = 0; i < count; ++i) {
        NoteParticle pt;
        pt.x = note.left() + rng->generateDouble() * qMax(note.width(), 1);
        pt.y = note.top() + rng->generateDouble() * qMax(note.height(), 1);
        const qreal angle = rng->generateDouble() * 2 * M_PI;
        const qreal speed = 8.0 + rng->generateDouble() * 14.0;
        pt.vx = std::cos(angle) * speed;
        pt.vy = std::sin(angle) * speed;
        pt.size = 1.2 + rng->generateDouble() * 1.4;
        pt.colorIdx = i;
        m_particles.append(pt);
    }
}

void DesktopWidget::advanceNoteParticles(qreal dt)
{
    const QRect note = noteRect();
    if (note.size() != m_particleArea || m_particles.isEmpty()) {
        initNoteParticles();
        return;
    }

    for (NoteParticle &pt : m_particles) {
        pt.x += pt.vx * dt;
        pt.y += pt.vy * dt;

        // 玻璃纸内反弹
        if (pt.x < note.left())       { pt.x = note.left();       pt.vx = -pt.vx; }
        if (pt.x > note.right())      { pt.x = note.right();      pt.vx = -pt.vx; }
        if (pt.y < note.top())        { pt.y = note.top();        pt.vy = -pt.vy; }
        if (pt.y > note.bottom())     { pt.y = note.bottom();     pt.vy = -pt.vy; }
    }
}

void DesktopWidget::drawNote(QPainter &p)
{
    const QRect note = noteRect();

    // 深色柔和投影
    for (int i = 5; i >= 1; --i) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 12));
        p.drawRoundedRect(note.adjusted(-i, -i + 3, i, i + 3), 14 + i, 14 + i);
    }

    // 玻璃纸：深色为深空渐变，浅色为奶白磨砂渐变
    QLinearGradient grad(note.topLeft(), note.bottomLeft());
    if (Theme::isDark()) {
        grad.setColorAt(0, QColor(0x18, 0x1D, 0x33, 240));
        grad.setColorAt(1, QColor(0x0B, 0x0E, 0x1A, 240));
    } else {
        grad.setColorAt(0, QColor(0xFF, 0xFF, 0xFF, 243));
        grad.setColorAt(1, QColor(0xEF, 0xF3, 0xFA, 238));
    }
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(note, 14, 14);

    // 内部粒子动效（裁剪在玻璃纸内，漂浮 + 邻近连线，无鼠标交互省电）
    p.save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(note, 14, 14);
    p.setClipPath(clipPath);

    const int n = m_particles.size();
    for (int i = 0; i < n; ++i) {
        const NoteParticle &a = m_particles[i];
        for (int j = i + 1; j < n; ++j) {
            const NoteParticle &b = m_particles[j];
            const qreal dx = a.x - b.x;
            const qreal dy = a.y - b.y;
            const qreal d2 = dx * dx + dy * dy;
            if (d2 > 70.0 * 70.0) continue;
            const qreal closeness = 1.0 - std::sqrt(d2) / 70.0;
            QColor c = (a.colorIdx % 2 == 0) ? Theme::primary() : Theme::accent();
            c.setAlpha(static_cast<int>((Theme::isDark() ? 60 : 85) * closeness * closeness));
            p.setPen(QPen(c, 1));
            p.drawLine(QPointF(a.x, a.y), QPointF(b.x, b.y));
        }
    }
    for (const NoteParticle &pt : m_particles) {
        QColor c = (pt.colorIdx % 3 == 0) ? Theme::neonPink()
                 : (pt.colorIdx % 2 == 0) ? Theme::primary() : Theme::accent();
        c.setAlpha(Theme::isDark() ? 170 : 200);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size, pt.size);
    }
    p.restore();

    // 玻璃描边 + 顶部高光
    p.setPen(QPen(Theme::glassBorder(), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(note).adjusted(0.5, 0.5, -0.5, -0.5), 14, 14);
    p.setPen(QPen(Theme::glassHighlight(), 1));
    p.drawLine(note.left() + 16, note.top() + 1, note.right() - 16, note.top() + 1);

    // 顶部霓虹渐变细边
    QLinearGradient edgeGrad(note.topLeft(), note.topRight());
    QColor c1 = Theme::primary(), c2 = Theme::accent();
    c1.setAlpha(110); c2.setAlpha(110);
    edgeGrad.setColorAt(0, c1);
    edgeGrad.setColorAt(0.5, c2);
    edgeGrad.setColorAt(1, QColor(c1.red(), c1.green(), c1.blue(), 0));
    p.setPen(QPen(QBrush(edgeGrad), 2));
    p.drawLine(note.left() + 14, note.top() + 2, note.right() - 14, note.top() + 2);
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

    // 玻璃贴片底
    p.setPen(QPen(Theme::glassBorder(), 1));
    p.setBrush(Theme::glassBgStrong());
    p.drawRoundedRect(r, 12, 12);

    // 天气图标（霓虹配色）
    const QRectF iconBox(r.left() + 7, r.center().y() - 7.5, 15, 15);
    const int kind = (m_weatherCode < 0) ? WSun : weatherKind(m_weatherCode);
    const QColor cloudColor(0xC9, 0xD6, 0xE8);

    auto drawSun = [&](const QRectF &b) {
        p.setPen(QPen(Theme::warning(), 1.6, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Theme::withAlpha(Theme::warning(), 90));
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
        p.setPen(QPen(cloudColor, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Theme::withAlpha(cloudColor, 60));
        p.drawPath(cloudPath(b));
    };

    if (m_weatherLoading) {
        // 加载中：旋转小圆弧
        p.setPen(QPen(Theme::primary(), 1.6, Qt::SolidLine, Qt::RoundCap));
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
            p.setBrush(Theme::accent());
            p.drawPath(bolt);
        } else {
            p.setPen(QPen(Theme::primary(), 1.5, Qt::SolidLine, Qt::RoundCap));
            for (int i = 0; i < 3; ++i) {
                const qreal x = iconBox.left() + 4 + i * 4.5;
                p.drawLine(QPointF(x, iconBox.bottom() - 5), QPointF(x - 1.5, iconBox.bottom() - 1));
            }
        }
    } else if (kind == WSnow) {
        drawCloud(QRectF(iconBox.left(), iconBox.top(), iconBox.width(), iconBox.height() - 5));
        p.setPen(QPen(QColor(0x67, 0xE8, 0xF9), 1.3, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 2; ++i) {
            const QPointF c(iconBox.left() + 5 + i * 7, iconBox.bottom() - 2);
            p.drawLine(c + QPointF(-2, 0), c + QPointF(2, 0));
            p.drawLine(c + QPointF(0, -2), c + QPointF(0, 2));
        }
    } else { // WFog
        drawCloud(QRectF(iconBox.left(), iconBox.top(), iconBox.width(), iconBox.height() - 6));
        p.setPen(QPen(Theme::textMuted(), 1.3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(iconBox.left() + 2, iconBox.bottom() - 3), QPointF(iconBox.right() - 4, iconBox.bottom() - 3));
        p.drawLine(QPointF(iconBox.left() + 5, iconBox.bottom()), QPointF(iconBox.right() - 1, iconBox.bottom()));
    }

    // 文字（城市 + 温度）
    QFont f(QStringLiteral("Microsoft YaHei UI"), -1);
    f.setPixelSize(12);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    p.setPen(Theme::textPrimary());
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
    const QString trimmed = MessageUtils::getText(this, QStringLiteral("设置城市"),
                                                  QStringLiteral("输入城市名（如：上海）："),
                                                  m_weatherCity).trimmed();
    if (!trimmed.isEmpty() && trimmed != m_weatherCity) {
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
    // 双击空白：聚焦快速添加
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
    const QString menuBg = Theme::isDark() ? QStringLiteral("rgba(20, 24, 42, 245)")
                                           : QStringLiteral("rgba(255, 255, 255, 247)");
    menu.setStyleSheet(QStringLiteral(
        "QMenu { background-color: %4; border: 1px solid %1;"
        "        border-radius: 8px; padding: 4px; }"
        "QMenu::item { padding: 7px 24px; border-radius: 4px; color: %2; }"
        "QMenu::item:selected { background-color: %3; }"
        "QMenu::item:checked { font-weight: 600; }")
        .arg(Theme::glassBorder().name(QColor::HexArgb),
             Theme::textPrimary().name(),
             Theme::hoverGlow().name(QColor::HexArgb),
             menuBg));

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
    settings.setValue(QStringLiteral("DesktopWidget/opacity"), windowOpacity());
    settings.setValue(QStringLiteral("DesktopWidget/onTop"), m_alwaysOnTop);
    settings.setValue(QStringLiteral("DesktopWidget/weatherCity"), m_weatherCity);
}

void DesktopWidget::loadAppearance()
{
    QSettings settings;
    const qreal opacity = qBound(0.3, settings.value(QStringLiteral("DesktopWidget/opacity"), 1.0).toReal(), 1.0);
    m_alwaysOnTop = settings.value(QStringLiteral("DesktopWidget/onTop"), true).toBool();
    m_weatherCity = settings.value(QStringLiteral("DesktopWidget/weatherCity"), QStringLiteral("北京")).toString();

    setWindowOpacity(opacity);
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool;
    if (m_alwaysOnTop) flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
}
