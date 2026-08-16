#include "navbar.h"
#include "../theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QEasingCurve>

namespace {

// 页面切换过渡层：内容淡入淡出，背景（极光粒子）全程保持不变。
// 旧页内容先淡出 → 只剩背景 → 新页内容再淡入；两页内容任何时刻都不叠加。
// 过渡期间新页面被临时隐藏（下方透出实时背景），动画结束后恢复显示。
// 动画期每帧仅 1 次贴图 blit，无布局/重绘开销。
class PageFadeOverlay : public QWidget
{
public:
    // parent = 页面容器（QStackedWidget）；hiddenPage = 过渡期间临时隐藏的新页
    PageFadeOverlay(const QPixmap &oldShot, const QPixmap &newShot,
                    QWidget *hiddenPage, QWidget *parent)
        : QWidget(parent)
        , m_old(oldShot)
        , m_new(newShot)
        , m_hiddenPage(hiddenPage)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setGeometry(parent->rect());
        show();
        raise();

        auto *anim = new QVariantAnimation(this);
        anim->setDuration(360);              // 两段各 ~180ms
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
            m_t = v.toReal();
            update();
        });
        connect(anim, &QVariantAnimation::finished, this, [this]() {
            restorePage();
            deleteLater();
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 恢复被临时隐藏的页面（动画正常结束或被打断时都要调用）
    void restorePage()
    {
        if (m_hiddenPage) {
            m_hiddenPage->setVisible(true);
            m_hiddenPage = nullptr;
        }
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        if (m_t < 0.5) {
            // 阶段一：旧页内容加速淡出（露出背景）
            const qreal k = m_t * 2.0;
            p.setOpacity(1.0 - k * k);              // InQuad：起步柔、收尾快
            p.drawPixmap(0, 0, m_old);
        } else {
            // 阶段二：新页内容从背景中减速淡入
            const qreal k = (m_t - 0.5) * 2.0;
            p.setOpacity(1.0 - (1.0 - k) * (1.0 - k));  // OutQuad：起步快、收尾柔
            p.drawPixmap(0, 0, m_new);
        }
    }

private:
    QPixmap m_old;
    QPixmap m_new;
    QPointer<QWidget> m_hiddenPage;
    qreal m_t = 0.0;
};

} // namespace

NavBar::NavBar(const QString &appName, const QStringList &items, QWidget *parent)
    : QWidget(parent)
    , m_appName(appName)
    , m_items(items)
{
    setFixedHeight(64);
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);
    setAttribute(Qt::WA_TranslucentBackground);

    m_rightContainer = new QWidget(this);
    m_rightContainer->setStyleSheet(QStringLiteral("background: transparent;"));
    m_rightLayout = new QHBoxLayout(m_rightContainer);
    m_rightLayout->setContentsMargins(0, 0, 20, 0);
    m_rightLayout->setSpacing(8);
}

void NavBar::attachStack(QStackedWidget *stack)
{
    m_stack = stack;
}

void NavBar::addRightWidget(QWidget *w)
{
    m_rightLayout->addWidget(w);
}

QRect NavBar::itemRect(int index) const
{
    // 导航项整体居中
    QFontMetrics fm(Theme::font(Theme::fontMedium, QFont::Medium));
    int totalWidth = 0;
    QVector<int> widths;
    for (const QString &it : m_items) {
        int w = fm.horizontalAdvance(it) + 44;
        widths.append(w);
        totalWidth += w;
    }
    int x = (width() - totalWidth) / 2;
    for (int i = 0; i < index; ++i) {
        x += widths[i];
    }
    return QRect(x, 0, widths.value(index, 0), height());
}

QRectF NavBar::capsuleRect(int index) const
{
    // 选中胶囊：包裹文字，垂直居中，上下留 10px
    return QRectF(itemRect(index)).adjusted(10, 10, -10, -10);
}

void NavBar::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_items.size() || index == m_currentIndex) {
        return;
    }
    m_currentIndex = index;
    moveIndicator(index, true);
    slideToPage(index);
    emit pageSelected(index);
    update();
}

void NavBar::moveIndicator(int index, bool animated)
{
    const QRectF target = capsuleRect(index);

    if (animated) {
        if (!m_indAnim) {
            m_indAnim = new QVariantAnimation(this);
            m_indAnim->setEasingCurve(QEasingCurve::OutCubic);
            connect(m_indAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
                m_indRect = v.toRectF();
                update();
            });
        }
        m_indAnim->stop();
        m_indAnim->setDuration(300);
        m_indAnim->setStartValue(m_indRect.isValid() ? m_indRect : target);
        m_indAnim->setEndValue(target);
        m_indAnim->start();
    } else {
        if (m_indAnim) m_indAnim->stop();
        m_indRect = target;
        update();
    }
}

void NavBar::slideToPage(int index)
{
    if (!m_stack || index >= m_stack->count()) {
        return;
    }

    // 打断进行中的过渡：先恢复被临时隐藏的页面并销毁旧过渡层
    if (m_fadeOverlay) {
        static_cast<PageFadeOverlay *>(m_fadeOverlay.data())->restorePage();
        delete m_fadeOverlay.data();
        m_fadeOverlay = nullptr;
        m_fadeHiddenPage = nullptr;
    }

    // 快照抓取：不能用 grab()——它对无 WA_TranslucentBackground 的控件
    // 会用调色板默认底色（白）填充未绘制区域，暗色模式下就是白屏。
    // 手动透明底 + render，保留页面真实的透明区域。
    auto renderPage = [](QWidget *w) -> QPixmap {
        if (!w || w->width() <= 0 || w->height() <= 0) {
            return QPixmap();
        }
        const qreal dpr = w->devicePixelRatioF();
        QPixmap pm(w->size() * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        w->render(&p, QPoint(), QRegion(), QWidget::DrawChildren);
        return pm;
    };

    // 切页前抓旧页快照（之后旧页会被隐藏）
    QWidget *oldPage = m_stack->currentWidget();
    QPixmap oldShot;
    if (oldPage && oldPage->isVisible()) {
        oldShot = renderPage(oldPage);
    }

    m_stack->setCurrentIndex(index);
    QWidget *page = m_stack->currentWidget();
    if (!page) {
        return;
    }

    // 切页后新页已可见，抓新页快照
    const QPixmap newShot = renderPage(page);
    if (oldShot.isNull() || newShot.isNull()) {
        return;
    }

    // 过渡期间临时隐藏新页（下方透出实时背景），内容淡入淡出、背景不变
    page->setVisible(false);
    auto *overlay = new PageFadeOverlay(oldShot, newShot, page, m_stack);   // 自管理
    m_fadeOverlay = overlay;
    m_fadeHiddenPage = page;
    connect(overlay, &QObject::destroyed, this, [this]() {
        m_fadeOverlay = nullptr;
        m_fadeHiddenPage = nullptr;
    });
}

void NavBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_rightContainer->adjustSize();
    m_rightContainer->move(width() - m_rightContainer->width(), 0);
    m_rightContainer->setFixedHeight(height());
    moveIndicator(m_currentIndex, false);
}

void NavBar::mousePressEvent(QMouseEvent *event)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (itemRect(i).contains(event->pos())) {
            setCurrentIndex(i);
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void NavBar::mouseMoveEvent(QMouseEvent *event)
{
    int hover = -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (itemRect(i).contains(event->pos())) {
            hover = i;
            break;
        }
    }
    if (hover != m_hoveredIndex) {
        m_hoveredIndex = hover;
        setCursor(hover >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void NavBar::leaveEvent(QEvent *event)
{
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        setCursor(Qt::ArrowCursor);
        update();
    }
    QWidget::leaveEvent(event);
}

void NavBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 玻璃底（透出极光背景）
    QColor bg = Theme::isDark() ? QColor(0x0B, 0x0E, 0x1A, 130) : QColor(255, 255, 255, 150);
    painter.fillRect(rect(), bg);
    painter.setPen(Theme::glassBorder());
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());

    // 左侧应用名（为空时跳过，避免与标题栏重复）
    if (!m_appName.isEmpty()) {
        painter.setFont(Theme::font(Theme::fontTitle, QFont::Bold));
        painter.setPen(Theme::textPrimary());
        painter.drawText(QRect(24, 0, 260, height()), Qt::AlignLeft | Qt::AlignVCenter, m_appName);
    }

    // 选中胶囊：半透明主题色底 + 霓虹渐变描边（先画，垫在文字下）
    if (m_indRect.isValid()) {
        const qreal radius = m_indRect.height() / 2.0;

        // 外侧柔光
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::withAlpha(Theme::primary(), Theme::isDark() ? 18 : 12));
        painter.drawRoundedRect(m_indRect.adjusted(-2, -2, 2, 2), radius + 2, radius + 2);

        // 胶囊底
        painter.setBrush(Theme::withAlpha(Theme::primary(), Theme::isDark() ? 34 : 24));
        painter.drawRoundedRect(m_indRect, radius, radius);

        // 渐变描边（青 -> 紫）
        QLinearGradient borderGrad(m_indRect.topLeft(), m_indRect.topRight());
        borderGrad.setColorAt(0, Theme::withAlpha(Theme::primary(), 220));
        borderGrad.setColorAt(1, Theme::withAlpha(Theme::accent(), 220));
        painter.setPen(QPen(QBrush(borderGrad), 1.2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(m_indRect.adjusted(0.6, 0.6, -0.6, -0.6), radius - 0.6, radius - 0.6);
    }

    // 导航项文字
    for (int i = 0; i < m_items.size(); ++i) {
        QRect r = itemRect(i);
        const bool selected = (i == m_currentIndex);
        const bool hovered  = (i == m_hoveredIndex);

        // 悬停：文字后方光晕胶囊
        if (hovered && !selected) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::hoverGlow());
            painter.drawRoundedRect(QRectF(r).adjusted(10, 10, -10, -10),
                                    (r.height() - 20) / 2.0, (r.height() - 20) / 2.0);
        }

        QFont f = Theme::font(Theme::fontMedium, selected ? QFont::DemiBold : QFont::Medium);
        painter.setFont(f);

        if (selected) {
            // 霓虹渐变文字
            QLinearGradient grad(r.topLeft(), r.bottomRight());
            grad.setColorAt(0, Theme::primary());
            grad.setColorAt(1, Theme::accent());
            painter.setPen(QPen(QBrush(grad), 1));
        } else {
            painter.setPen(hovered ? Theme::textPrimary() : Theme::textSecondary());
        }
        painter.drawText(r, Qt::AlignCenter, m_items[i]);
    }
}

// ---------------- NavIconButton ----------------

NavIconButton::NavIconButton(Icons::Type type, const QString &tooltip, QWidget *parent)
    : QPushButton(parent)
{
    // ghost 变体仅用于清除 QSS 基础按钮样式干扰（min-width 等），视觉全部自绘
    setProperty("variant", "ghost");
    setFixedSize(36, 36);
    setToolTip(tooltip);
    setCursor(Qt::PointingHandCursor);
    m_pmNormal = Icons::pixmap(type, 18, Theme::textSecondary());
    m_pmHover  = Icons::pixmap(type, 18, Theme::primary());
}

void NavIconButton::animateTo(qreal target, int duration)
{
    if (!m_anim) {
        m_anim = new QVariantAnimation(this);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
            m_hover = v.toReal();
            update();
        });
    }
    m_anim->stop();
    m_anim->setDuration(duration);
    m_anim->setStartValue(m_hover);
    m_anim->setEndValue(target);
    m_anim->start();
}

void NavIconButton::enterEvent(QEnterEvent *event)
{
    animateTo(1.0, 160);
    QPushButton::enterEvent(event);
}

void NavIconButton::leaveEvent(QEvent *event)
{
    animateTo(0.0, 240);
    QPushButton::leaveEvent(event);
}

void NavIconButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
    const qreal radius = 10.0;

    // 按下态：比悬停更深一档
    const qreal intensity = isDown() ? qMin<qreal>(1.0, m_hover + 0.35) : m_hover;

    if (intensity > 0.01) {
        // 光晕底
        QColor fill = Theme::primary();
        fill.setAlpha(static_cast<int>((Theme::isDark() ? 34 : 26) * intensity));
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawRoundedRect(r, radius, radius);

        // 霓虹描边
        QColor edge = Theme::primary();
        edge.setAlpha(static_cast<int>(150 * intensity));
        p.setPen(QPen(edge, 1.1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(0.55, 0.55, -0.55, -0.55), radius - 0.5, radius - 0.5);
    }

    // 图标：常态灰 -> 悬停主题色，按进度叠加
    const QPoint pos((width() - m_pmNormal.width() / m_pmNormal.devicePixelRatio()) / 2,
                     (height() - m_pmNormal.height() / m_pmNormal.devicePixelRatio()) / 2);
    p.drawPixmap(pos, m_pmNormal);
    if (intensity > 0.01) {
        p.setOpacity(intensity);
        p.drawPixmap(pos, m_pmHover);
        p.setOpacity(1.0);
    }
}
