#include "titlebar.h"
#include "../theme.h"

#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QMouseEvent>
#include <QWindow>
#include <QApplication>

namespace {
constexpr int kBarHeight = 40;
constexpr int kBtnWidth  = 44;

// 按钮图标字形绘制（min / max / restore / close）
void drawGlyph(QPainter &p, const QRect &r, int btn, bool maximized, const QColor &color)
{
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 1.4, Qt::SolidLine, Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const QPoint c = r.center();

    if (btn == 0) {                       // 最小化：横线
        p.drawLine(c.x() - 5, c.y(), c.x() + 5, c.y());
    } else if (btn == 1) {                // 最大化 / 还原
        if (maximized) {                  // 还原：两个错位方框
            QRect back(c.x() - 3, c.y() - 5, 7, 7);
            QRect front(c.x() - 5, c.y() - 2, 7, 7);
            p.drawRect(back);
            p.drawRect(front);
        } else {                          // 最大化：单个方框
            p.drawRect(QRect(c.x() - 5, c.y() - 5, 10, 10));
        }
    } else {                              // 关闭：叉
        p.drawLine(c.x() - 5, c.y() - 5, c.x() + 5, c.y() + 5);
        p.drawLine(c.x() - 5, c.y() + 5, c.x() + 5, c.y() - 5);
    }
}
} // namespace

TitleBar::TitleBar(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
{
    setFixedHeight(kBarHeight);
    setMouseTracking(true);
}

void TitleBar::setTitle(const QString &title)
{
    m_title = title;
    update();
}

QRect TitleBar::buttonRect(Button btn) const
{
    // 从右往左：close, max, min
    int indexFromRight = BtnClose - btn;
    return QRect(width() - kBtnWidth * (indexFromRight + 1), 0, kBtnWidth, kBarHeight);
}

TitleBar::Button TitleBar::buttonAt(const QPoint &pos) const
{
    for (int b = BtnMin; b <= BtnClose; ++b) {
        if (buttonRect(static_cast<Button>(b)).contains(pos)) {
            return static_cast<Button>(b);
        }
    }
    return BtnNone;
}

void TitleBar::toggleMaxRestore()
{
    QWidget *w = window();
    if (!w) return;
    if (w->isMaximized()) {
        w->showNormal();
    } else {
        w->showMaximized();
    }
    update();
}

void TitleBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 背景
    p.fillRect(rect(), Theme::surface());
    p.setPen(Theme::border());
    p.drawLine(rect().bottomLeft(), rect().bottomRight());

    // 应用图标
    QPixmap icon = QIcon(QStringLiteral(":/icons/app.png")).pixmap(18, 18);
    if (icon.isNull()) {
        icon = QIcon(QStringLiteral(":/icons/app.ico")).pixmap(18, 18);
    }
    int textX = 14;
    if (!icon.isNull()) {
        p.drawPixmap(14, (kBarHeight - 18) / 2, icon);
        textX = 14 + 18 + 8;
    }

    // 标题
    p.setFont(Theme::font(Theme::fontBase, QFont::DemiBold));
    p.setPen(Theme::textPrimary());
    int textRight = buttonRect(BtnMin).left() - 8;
    p.drawText(QRect(textX, 0, qMax(textRight - textX, 0), kBarHeight),
               Qt::AlignLeft | Qt::AlignVCenter, m_title);

    // 窗口控制按钮
    QWidget *w = window();
    const bool maximized = w && w->isMaximized();

    for (int b = BtnMin; b <= BtnClose; ++b) {
        QRect r = buttonRect(static_cast<Button>(b));
        QColor glyphColor = Theme::textSecondary();

        if (m_pressedBtn == b && m_hoverBtn == b) {
            p.fillRect(r, b == BtnClose ? QColor(0xC8, 0x11, 0x22) : QColor(0, 0, 0, 26));
            if (b == BtnClose) glyphColor = Qt::white;
        } else if (m_hoverBtn == b) {
            p.fillRect(r, b == BtnClose ? QColor(0xE8, 0x11, 0x23) : QColor(0, 0, 0, 14));
            if (b == BtnClose) glyphColor = Qt::white;
        }
        drawGlyph(p, r, b, maximized, glyphColor);
    }
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressedBtn = buttonAt(event->pos());
        if (m_pressedBtn == BtnNone) {
            // 空白区域：开始拖动
            m_dragging = true;
            m_dragOffset = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        }
        update();
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        QWidget *w = window();
        if (w && !w->isMaximized()) {
            w->move(event->globalPosition().toPoint() - m_dragOffset);
        }
        return;
    }

    int hover = buttonAt(event->pos());
    if (hover != m_hoverBtn) {
        m_hoverBtn = hover;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int released = buttonAt(event->pos());
        if (m_pressedBtn != BtnNone && released == m_pressedBtn) {
            QWidget *w = window();
            switch (m_pressedBtn) {
            case BtnMin:   if (w) w->showMinimized(); break;
            case BtnMax:   toggleMaxRestore(); break;
            case BtnClose: if (w) w->close(); break;
            default: break;
            }
        }
        m_pressedBtn = BtnNone;
        m_dragging = false;
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && buttonAt(event->pos()) == BtnNone) {
        toggleMaxRestore();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::leaveEvent(QEvent *event)
{
    if (m_hoverBtn != BtnNone) {
        m_hoverBtn = BtnNone;
        update();
    }
    QWidget::leaveEvent(event);
}
