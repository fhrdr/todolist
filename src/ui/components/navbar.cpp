#include "navbar.h"
#include "../theme.h"

#include <QPainter>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>

NavBar::NavBar(const QString &appName, const QStringList &items, QWidget *parent)
    : QWidget(parent)
    , m_appName(appName)
    , m_items(items)
{
    setFixedHeight(64);
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);

    m_indicator = new QWidget(this);
    m_indicator->setFixedHeight(4);
    m_indicator->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 2px;")
                               .arg(Theme::primary().name()));

    m_rightContainer = new QWidget(this);
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

void NavBar::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_items.size() || index == m_currentIndex) {
        return;
    }
    m_currentIndex = index;
    moveIndicator(index, true);
    fadeToPage(index);
    emit pageSelected(index);
    update();
}

void NavBar::moveIndicator(int index, bool animated)
{
    QRect r = itemRect(index);
    int iw = r.width() - 36;
    QRect target(r.center().x() - iw / 2, height() - 12, iw, 4);

    if (animated) {
        if (!m_indicatorAnim) {
            m_indicatorAnim = new QPropertyAnimation(m_indicator, "geometry", this);
            m_indicatorAnim->setEasingCurve(QEasingCurve::OutCubic);
        }
        m_indicatorAnim->stop();
        m_indicatorAnim->setDuration(320);
        m_indicatorAnim->setStartValue(m_indicator->geometry());
        m_indicatorAnim->setEndValue(target);
        m_indicatorAnim->start();
    } else {
        m_indicator->setGeometry(target);
    }
}

void NavBar::fadeToPage(int index)
{
    if (!m_stack || index >= m_stack->count()) {
        return;
    }
    m_stack->setCurrentIndex(index);
    QWidget *page = m_stack->currentWidget();
    if (!page) {
        return;
    }
    auto *effect = new QGraphicsOpacityEffect(page);
    page->setGraphicsEffect(effect);
    auto *anim = new QPropertyAnimation(effect, "opacity", page);
    anim->setDuration(280);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
    // 动画结束后移除 effect，避免长期占用
    QObject::connect(anim, &QPropertyAnimation::finished, page, [page]() {
        page->setGraphicsEffect(nullptr);
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

void NavBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 背景
    painter.fillRect(rect(), Theme::surface());
    painter.setPen(Theme::border());
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());

    // 左侧应用名（为空时跳过，避免与标题栏重复）
    if (!m_appName.isEmpty()) {
        painter.setFont(Theme::font(Theme::fontTitle, QFont::Bold));
        painter.setPen(Theme::textPrimary());
        painter.drawText(QRect(24, 0, 260, height()), Qt::AlignLeft | Qt::AlignVCenter, m_appName);
    }

    // 导航项文字
    QFontMetrics fm(Theme::font(Theme::fontMedium, QFont::Medium));
    for (int i = 0; i < m_items.size(); ++i) {
        QRect r = itemRect(i);
        bool selected = (i == m_currentIndex);

        QFont f = Theme::font(Theme::fontMedium, selected ? QFont::DemiBold : QFont::Medium);
        painter.setFont(f);
        painter.setPen(selected ? Theme::primary() : Theme::textSecondary());
        painter.drawText(r, Qt::AlignCenter, m_items[i]);
    }
}
