#include "sectionheader.h"
#include "../theme.h"

#include <QPainter>
#include <QFontMetrics>
#include <QShowEvent>
#include <QEasingCurve>

SectionHeader::SectionHeader(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SectionHeader::setTitle(const QString &title)
{
    m_title = title;
    updateGeometry();
    update();
}

QSize SectionHeader::sizeHint() const
{
    QFontMetrics fm(Theme::font(Theme::fontTitle, QFont::DemiBold));
    return QSize(fm.horizontalAdvance(m_title) + 16, fm.height() + 16);
}

void SectionHeader::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_animated) {
        m_animated = true;
        QPropertyAnimation *anim = new QPropertyAnimation(this, "underlineProgress", this);
        anim->setDuration(420);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    } else if (m_progress < 1.0) {
        m_progress = 1.0;
    }
}

void SectionHeader::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont f = Theme::font(Theme::fontTitle, QFont::DemiBold);
    painter.setFont(f);
    painter.setPen(Theme::textPrimary());

    QFontMetrics fm(f);
    int textWidth = fm.horizontalAdvance(m_title);
    QRect textRect(0, 0, width(), fm.height() + 4);
    painter.drawText(textRect, Qt::AlignCenter, m_title);

    // 下划线：加粗、紧贴文字，宽度跟随动画进度从中心展开
    int lineWidth = static_cast<int>(qMin(textWidth + 8, width() - 16) * m_progress);
    if (lineWidth > 0) {
        int cx = width() / 2;
        int y = fm.height() + 8;
        QRect lineRect(cx - lineWidth / 2, y, lineWidth, 4);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::primary());
        painter.drawRoundedRect(lineRect, 2, 2);
    }
}
