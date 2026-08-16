#include "statswidget.h"
#include "../theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QScrollArea>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QToolTip>
#include <algorithm>

// ---------------- HoverState（悬浮上浮动效） ----------------

void HoverState::attach(QWidget *w)
{
    anim = new QVariantAnimation(w);
    anim->setDuration(180);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(anim, &QVariantAnimation::valueChanged, w, [this, w](const QVariant &v) {
        value = v.toReal();
        w->update();
    });
}

void HoverState::start(qreal target)
{
    if (!anim) return;
    anim->stop();
    anim->setStartValue(value);
    anim->setEndValue(target);
    anim->start();
}

void HoverState::enter(QEnterEvent *)  { start(1.0); }
void HoverState::leave(QEvent *)       { start(0.0); }

namespace {
constexpr int kCardPadding = 20;   // 卡片内边距
constexpr int kCardSpacing = 14;   // 卡片间距
constexpr int kPageMargin  = 20;   // 页面外边距
constexpr int kChartDays   = 14;   // 柱状图统计天数
constexpr int kHeatmapDays = 126;  // 热力图统计天数（18 周）
constexpr int kAnimMs      = 800;  // 数字/入场动画时长

// 统一的玻璃卡片容器：半透明底 + 高光描边 + 顶部高光条
void drawGlassCard(QPainter &painter, const QRect &rect)
{
    const QRectF r = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::glassBg());
    painter.drawRoundedRect(r, Theme::radiusLg, Theme::radiusLg);

    painter.setPen(QPen(Theme::glassBorder(), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(r, Theme::radiusLg, Theme::radiusLg);

    // 顶部高光条
    painter.setPen(QPen(Theme::glassHighlight(), 1));
    painter.drawLine(QPointF(r.left() + Theme::radiusLg, r.top()),
                     QPointF(r.right() - Theme::radiusLg, r.top()));
}

void drawCardTitle(QPainter &painter, const QString &title, const QRect &rect)
{
    // 标题左侧加一条霓虹渐变竖线点缀
    QLinearGradient barGrad(rect.left(), rect.top(), rect.left(), rect.bottom());
    barGrad.setColorAt(0, Theme::primary());
    barGrad.setColorAt(1, Theme::accent());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(barGrad));
    painter.drawRoundedRect(QRectF(rect.left(), rect.top() + 3, 4, rect.height() - 6), 2, 2);

    painter.setFont(Theme::font(Theme::fontMedium, QFont::DemiBold));
    painter.setPen(Theme::textPrimary());
    painter.drawText(rect.adjusted(14, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, title);
}

// 渐变文字：通过 QPainterPath 填充线性渐变
void drawGradientText(QPainter &painter, const QRect &rect, const QFont &font,
                      const QString &text, const QColor &from, const QColor &to)
{
    QPainterPath path;
    const QFontMetrics fm(font);
    const int x = rect.left() + (rect.width() - fm.horizontalAdvance(text)) / 2;
    const int y = rect.top() + (rect.height() - fm.height()) / 2 + fm.ascent();
    path.addText(x, y, font, text);

    QLinearGradient grad(rect.topLeft(), rect.bottomRight());
    grad.setColorAt(0, from);
    grad.setColorAt(1, to);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(grad));
    painter.drawPath(path);
}

// 卡片绘制入口：悬浮时卡片框架上边缘轻微上移（顶部预留余量，描边不被裁剪）
void beginCardPaint(QPainter &painter, const QRect &rect, qreal hover)
{
    const int lift = static_cast<int>(3.0 * hover + 0.5);
    const QRect cardRect = rect.adjusted(0, 4 - lift, 0, 0);
    drawGlassCard(painter, cardRect);
    if (hover > 0.01) {
        QColor g = Theme::primary();
        g.setAlpha(static_cast<int>(80 * hover));
        painter.setPen(QPen(g, 1.4));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(cardRect).adjusted(0.5, 0.5, -0.5, -0.5),
                                Theme::radiusLg, Theme::radiusLg);
    }
}
} // namespace

// ---------------- StatsOverviewCard ----------------

StatsOverviewCard::StatsOverviewCard(const QString &caption,
                                     const QColor &gradFrom, const QColor &gradTo,
                                     QWidget *parent)
    : QWidget(parent)
    , m_caption(caption)
    , m_gradFrom(gradFrom)
    , m_gradTo(gradTo)
{
    setFixedHeight(118);
    setMinimumWidth(150);
    m_hover.attach(this);

    m_anim = new QVariantAnimation(this);
    m_anim->setDuration(kAnimMs);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_displayValue = v.toReal();
        update();
    });
}

void StatsOverviewCard::setValue(int value)
{
    if (value == m_value && m_anim->state() != QAbstractAnimation::Running)
        return;
    m_value = value;
    m_anim->stop();
    m_anim->setStartValue(m_displayValue);
    m_anim->setEndValue(static_cast<qreal>(value));
    m_anim->start();
}

void StatsOverviewCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    beginCardPaint(painter, rect(), m_hover.value);

    const int pad = kCardPadding - 4;

    // 右上角装饰光晕
    QRadialGradient glow(QPointF(width() - 26, 26), 40);
    QColor glowColor = Theme::withAlpha(m_gradFrom, Theme::isDark() ? 70 : 40);
    glow.setColorAt(0, glowColor);
    glow.setColorAt(1, Qt::transparent);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(glow));
    painter.drawEllipse(QPointF(width() - 26, 26), 40, 40);

    // 标题
    painter.setFont(Theme::font(Theme::fontSmall + 1));
    painter.setPen(Theme::textSecondary());
    painter.drawText(QRect(pad, 14, width() - 2 * pad, 20),
                     Qt::AlignLeft | Qt::AlignVCenter, m_caption);

    // 霓虹渐变大数字（滚动动画）
    drawGradientText(painter, QRect(pad, 36, width() - 2 * pad, 50),
                     Theme::font(Theme::fontHero + 12, QFont::Bold),
                     QString::number(qRound(m_displayValue)), m_gradFrom, m_gradTo);

    // 底部渐变短线
    QLinearGradient lineGrad(pad, 0, pad + 56, 0);
    lineGrad.setColorAt(0, m_gradFrom);
    lineGrad.setColorAt(1, Theme::withAlpha(m_gradTo, 0));
    painter.setBrush(QBrush(lineGrad));
    painter.drawRoundedRect(QRectF(pad, height() - 20, 56, 4), 2, 2);
}

// ---------------- StatsRingCard ----------------

StatsRingCard::StatsRingCard(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(260);
    setMinimumWidth(260);
    m_hover.attach(this);

    m_anim = new QVariantAnimation(this);
    m_anim->setDuration(kAnimMs + 300);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_progress = v.toReal();
        update();
    });
}

void StatsRingCard::setRate(int completed, int total)
{
    m_completed = completed;
    m_total = total;
    const qreal target = total > 0 ? qBound<qreal>(0.0, (qreal)completed / total, 1.0) : 0.0;
    m_anim->stop();
    m_anim->setStartValue(m_progress);
    m_anim->setEndValue(target);
    m_anim->start();
}

void StatsRingCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    beginCardPaint(painter, rect(), m_hover.value);

    const int pad = kCardPadding;
    drawCardTitle(painter, QStringLiteral("总体完成率"), QRect(pad, pad, width() - 2 * pad, 22));

    // 圆环区域：标题下方居中
    const QRect content(pad, pad + 34, width() - 2 * pad, height() - 2 * pad - 34);
    const int side = qMin(content.width(), content.height()) - 12;
    const QRectF ringRect(content.center().x() - side / 2.0,
                          content.center().y() - side / 2.0, side, side);
    const int penW = qMax(10, side / 14);

    // 轨道
    painter.setPen(QPen(Theme::withAlpha(Theme::glassBorder(), 130), penW,
                        Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(ringRect, 0, 360 * 16);

    // 进度弧：青 -> 紫 锥形渐变 + 发光底层
    if (m_progress > 0.001) {
        const int span = static_cast<int>(-m_progress * 360 * 16);

        QConicalGradient conic(ringRect.center(), 90);
        conic.setColorAt(0.0, Theme::primary());
        conic.setColorAt(1.0, Theme::accent());

        painter.setPen(QPen(Theme::withAlpha(Theme::primary(), 46), penW + 10,
                            Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(ringRect, 90 * 16, span);

        painter.setPen(QPen(QBrush(conic), penW, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(ringRect, 90 * 16, span);
    }

    // 中心百分比 + 分子分母
    const int percent = qRound(m_progress * 100);
    drawGradientText(painter, QRect(ringRect.left(), ringRect.top() + side * 0.22,
                                    side, side * 0.36),
                     Theme::font(Theme::fontHero + 10, QFont::Bold),
                     QStringLiteral("%1%").arg(percent),
                     Theme::primary(), Theme::accent());

    painter.setFont(Theme::font(Theme::fontSmall + 1));
    painter.setPen(Theme::textSecondary());
    painter.drawText(QRectF(ringRect.left(), ringRect.top() + side * 0.60, side, 22),
                     Qt::AlignHCenter | Qt::AlignVCenter,
                     QStringLiteral("%1 / %2 已完成").arg(m_completed).arg(m_total));
}

// ---------------- StatsBarChart ----------------

StatsBarChart::StatsBarChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(260);
    m_hover.attach(this);

    m_anim = new QVariantAnimation(this);
    m_anim->setDuration(kAnimMs);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_animProgress = v.toReal();
        update();
    });
}

void StatsBarChart::setDailyCounts(const QVector<QDate> &dates, const QVector<int> &counts)
{
    m_dates = dates;
    m_counts = counts;
    m_anim->stop();
    m_anim->setStartValue(0.0);
    m_anim->setEndValue(1.0);
    m_anim->start();
}

void StatsBarChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    beginCardPaint(painter, rect(), m_hover.value);

    const int pad = kCardPadding;
    drawCardTitle(painter, QStringLiteral("近 14 天完成"), QRect(pad, pad, width() - 2 * pad, 22));

    // 绘图区：左侧留 36px 给 Y 轴数值标签，底部留 22px 给 X 轴日期标签
    QRect plotRect(pad + 36, pad + 22 + 14, width() - 2 * pad - 36, 0);
    plotRect.setBottom(height() - pad - 22);

    const int days = m_counts.size();
    if (days <= 0 || plotRect.width() <= 0 || plotRect.height() <= 0)
        return;

    int maxCount = 0;
    for (int c : m_counts)
        maxCount = qMax(maxCount, c);

    if (maxCount == 0) {
        painter.setFont(Theme::font(Theme::fontBase));
        painter.setPen(Theme::textMuted());
        painter.drawText(plotRect, Qt::AlignCenter, QStringLiteral("近 14 天还没有完成记录"));
        return;
    }

    // Y 轴：最多 4 条淡色网格线
    const int step = (maxCount + 3) / 4;
    const int yMax = step * 4;

    painter.setFont(Theme::font(Theme::fontSmall));
    for (int i = 0; i <= 4; ++i) {
        const int value = step * i;
        const qreal y = plotRect.bottom() - (qreal)value / yMax * plotRect.height();
        painter.setPen(QPen(Theme::withAlpha(Theme::border(), 120), 1, Qt::DashLine));
        painter.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
        if (value > 0) {
            painter.setPen(Theme::textMuted());
            painter.drawText(QRect(pad, (int)y - 8, plotRect.left() - 6 - pad, 16),
                             Qt::AlignRight | Qt::AlignVCenter, QString::number(value));
        }
    }

    const qreal slotW = (qreal)plotRect.width() / days;
    const qreal barW = qMin<qreal>(slotW * 0.55, 34.0);

    for (int i = 0; i < days; ++i) {
        const int count = m_counts.at(i);
        const qreal x = plotRect.left() + i * slotW + (slotW - barW) / 2.0;
        const qreal bottom = plotRect.bottom();

        if (count > 0) {
            qreal barH = (qreal)count / yMax * plotRect.height() * m_animProgress;
            barH = qMax<qreal>(barH, 3.0);
            const qreal y = bottom - barH;
            const qreal r = qMin<qreal>(5.0, qMin(barW, barH) / 2.0);

            // 仅顶部圆角的柱子
            QPainterPath barPath;
            barPath.moveTo(x, bottom);
            barPath.lineTo(x, y + r);
            barPath.quadTo(x, y, x + r, y);
            barPath.lineTo(x + barW - r, y);
            barPath.quadTo(x + barW, y, x + barW, y + r);
            barPath.lineTo(x + barW, bottom);
            barPath.closeSubpath();

            // 发光底层
            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::withAlpha(Theme::primary(), 40));
            painter.drawPath(barPath.translated(0, -2));

            // 青 -> 紫 竖向渐变柱体
            QLinearGradient grad(x, y, x, bottom);
            grad.setColorAt(0, Theme::primary());
            grad.setColorAt(1, Theme::withAlpha(Theme::accent(), 200));
            painter.setBrush(QBrush(grad));
            painter.drawPath(barPath);

            // 顶部高光帽
            painter.setBrush(Theme::withAlpha(Theme::glassHighlight(), 160));
            painter.drawRoundedRect(QRectF(x + 2, y + 1, barW - 4, 3), 1.5, 1.5);

            painter.setFont(Theme::font(Theme::fontSmall));
            painter.setPen(Theme::textSecondary());
            painter.drawText(QRectF(x + barW / 2.0 - slotW / 2.0, y - 19, slotW, 16),
                             Qt::AlignHCenter | Qt::AlignVCenter, QString::number(count));
        }

        // X 轴日期标签：每隔一根显示一个
        if (i % 2 == 0 && i < m_dates.size()) {
            painter.setFont(Theme::font(Theme::fontSmall));
            painter.setPen(Theme::textMuted());
            painter.drawText(QRectF(plotRect.left() + i * slotW, plotRect.bottom() + 5, slotW, 17),
                             Qt::AlignHCenter | Qt::AlignTop,
                             m_dates.at(i).toString(QStringLiteral("M/d")));
        }
    }
}

// ---------------- StatsHeatmap ----------------

StatsHeatmap::StatsHeatmap(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(248);
    setMouseTracking(true);
    m_hover.attach(this);
}

void StatsHeatmap::setDailyMap(const QHash<QDate, int> &map)
{
    m_map = map;
    update();
}

StatsHeatmap::GridLayout StatsHeatmap::gridLayout() const
{
    GridLayout g;
    const int pad = kCardPadding;
    const int weekdayLabelW = 26;   // 左侧星期标签列宽
    const int monthLabelH   = 16;   // 顶部月份标签行高

    // 固定 kHeatmapDays 天（列 = 周）；格子尺寸随宽度自适应，过宽时居中显示
    g.cols = kHeatmapDays / 7;
    g.gap  = 4;
    const int availW = width() - 2 * pad - weekdayLabelW;
    g.cell = qBound(10, (availW + g.gap) / g.cols - g.gap, 18);

    const int gridW = g.cols * (g.cell + g.gap) - g.gap;
    g.gridX = pad + weekdayLabelW + qMax(0, (availW - gridW) / 2);
    g.gridY = pad + 34 + monthLabelH;

    // 周对齐：最后一列包含今天，每列从周一开始（Qt：周一 = 1）
    const QDate today = QDate::currentDate();
    g.firstDay = today.addDays(-((g.cols - 1) * 7 + (today.dayOfWeek() - 1)));
    return g;
}

QDate StatsHeatmap::dateAt(const QPoint &pos) const
{
    const GridLayout g = gridLayout();
    const int pitch = g.cell + g.gap;
    const int lx = pos.x() - g.gridX;
    const int ly = pos.y() - g.gridY;
    if (lx < 0 || ly < 0) return QDate();

    const int col = lx / pitch;
    const int row = ly / pitch;
    if (col >= g.cols || row >= 7) return QDate();
    // 落在格子间隙上不算命中
    if (lx % pitch > g.cell || ly % pitch > g.cell) return QDate();

    const QDate date = g.firstDay.addDays(col * 7 + row);
    if (date > QDate::currentDate()) return QDate();
    return date;
}

void StatsHeatmap::mouseMoveEvent(QMouseEvent *event)
{
    const QDate date = dateAt(event->pos());
    if (date != m_hoveredDate) {
        m_hoveredDate = date;
        update();
        if (date.isValid()) {
            const int count = m_map.value(date, 0);
            const QString tip = count > 0
                    ? QStringLiteral("%1 · 完成 %2 项").arg(date.toString(QStringLiteral("M月d日"))).arg(count)
                    : QStringLiteral("%1 · 无完成记录").arg(date.toString(QStringLiteral("M月d日")));
            QToolTip::showText(mapToGlobal(event->pos()), tip, this);
        } else {
            QToolTip::hideText();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void StatsHeatmap::leaveEvent(QEvent *event)
{
    m_hover.leave(event);
    if (m_hoveredDate.isValid()) {
        m_hoveredDate = QDate();
        QToolTip::hideText();
        update();
    }
    QWidget::leaveEvent(event);
}

void StatsHeatmap::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    beginCardPaint(painter, rect(), m_hover.value);

    const int pad = kCardPadding;
    drawCardTitle(painter, QStringLiteral("完成热力图"), QRect(pad, pad, width() - 2 * pad, 22));

    const GridLayout g = gridLayout();
    const int pitch = g.cell + g.gap;
    const QDate today = QDate::currentDate();

    int maxCount = 0;
    for (auto it = m_map.constBegin(); it != m_map.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());

    // ---- 顶部月份标签：月份变化的列上方显示，间隔不足则跳过 ----
    painter.setFont(Theme::font(Theme::fontSmall - 2));
    painter.setPen(Theme::textMuted());
    int lastLabelCol = -3;
    for (int c = 1; c < g.cols; ++c) {
        const QDate monday = g.firstDay.addDays(c * 7);
        if (monday.month() != g.firstDay.addDays((c - 1) * 7).month() && c - lastLabelCol >= 3) {
            painter.drawText(QRect(g.gridX + c * pitch - 4, pad + 32, 40, 14),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             QStringLiteral("%1月").arg(monday.month()));
            lastLabelCol = c;
        }
    }

    // ---- 左侧星期标签：一 / 三 / 五 ----
    static const char *weekdayNames[3] = { "一", "三", "五" };
    for (int i = 0; i < 3; ++i) {
        const int row = i * 2;
        painter.drawText(QRect(pad, g.gridY + row * pitch, g.gridX - pad - 6, g.cell),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::fromUtf8(weekdayNames[i]));
    }

    // ---- 方格：列 = 周（周一起始），行 = 星期 ----
    const qreal cellRadius = g.cell >= 15 ? 4.0 : 3.0;
    for (int c = 0; c < g.cols; ++c) {
        for (int r = 0; r < 7; ++r) {
            const QDate date = g.firstDay.addDays(c * 7 + r);
            if (date > today) break;    // 未来日期留空

            const QRectF cellRect(g.gridX + c * pitch, g.gridY + r * pitch, g.cell, g.cell);
            const int count = m_map.value(date, 0);
            if (count <= 0 || maxCount <= 0) {
                painter.setPen(QPen(Theme::withAlpha(Theme::glassBorder(), 110), 1));
                painter.setBrush(Theme::glassBg());
            } else {
                // 4 档霓虹色阶：越深 -> 越亮（伽马微调让中档更有层次）
                const qreal level = qMin<qreal>(1.0, (qreal)count / qMax(1, maxCount));
                QColor c = Theme::mix(Theme::primary(), Theme::accent(), level);
                c.setAlpha(Theme::isDark() ? (110 + (int)(145 * level))
                                           : (95 + (int)(140 * level)));
                painter.setPen(Qt::NoPen);
                painter.setBrush(c);
            }
            painter.drawRoundedRect(cellRect, cellRadius, cellRadius);

            // 悬停高亮描边
            if (date == m_hoveredDate) {
                painter.setPen(QPen(Theme::withAlpha(Theme::textPrimary(), 220), 1.4));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(cellRect.adjusted(-1.2, -1.2, 1.2, 1.2),
                                        cellRadius + 1, cellRadius + 1);
            }
        }
    }

    // ---- 底部图例：少 -> 多（跟随网格右缘对齐） ----
    painter.setFont(Theme::font(Theme::fontSmall));
    painter.setPen(Theme::textMuted());
    const QString lessText = QStringLiteral("少");
    const QString moreText = QStringLiteral("多");
    const QFontMetrics fm(painter.font());
    const int legendY = g.gridY + 7 * pitch - g.gap + 10;
    const int gridRight = g.gridX + g.cols * pitch - g.gap;
    int legendX = gridRight - fm.horizontalAdvance(lessText) - fm.horizontalAdvance(moreText)
                  - 5 * (10 + g.gap) - 16;
    painter.drawText(QRect(legendX, legendY, 30, 12), Qt::AlignLeft | Qt::AlignVCenter, lessText);
    legendX += fm.horizontalAdvance(lessText) + 6;
    for (int i = 0; i < 5; ++i) {
        const qreal t = i / 4.0;
        QColor c = Theme::mix(Theme::primary(), Theme::accent(), t);
        c.setAlpha(i == 0 ? 40 : (Theme::isDark() ? 110 + (int)(145 * t) : 95 + (int)(140 * t)));
        painter.setPen(i == 0 ? QPen(Theme::withAlpha(Theme::glassBorder(), 110), 1)
                              : QPen(Qt::NoPen));
        painter.setBrush(i == 0 ? Theme::glassBg() : c);
        painter.drawRoundedRect(QRectF(legendX, legendY + 1, 10, 10), 2.5, 2.5);
        legendX += 10 + g.gap;
    }
    painter.setPen(Theme::textMuted());
    painter.drawText(QRect(legendX + 2, legendY, 30, 12), Qt::AlignLeft | Qt::AlignVCenter, moreText);
}

// ---------------- StatsWidget ----------------

StatsWidget::StatsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void StatsWidget::setupUI()
{
    // 页面内容较多，外层包滚动区（透明，透出极光背景）
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }"));

    QWidget *content = new QWidget();
    content->setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(kPageMargin, kPageMargin, kPageMargin, kPageMargin);
    mainLayout->setSpacing(kCardSpacing);

    // 第一行：4 张概览卡（同色系渐变，色相区分语义：靛=总量 / 绿=完成 / 粉=今日 / 橙=连续）
    QHBoxLayout *overviewLayout = new QHBoxLayout();
    overviewLayout->setSpacing(kCardSpacing);
    m_totalCard = new StatsOverviewCard(QStringLiteral("总待办数"),
                                        Theme::primary(), Theme::accent(), content);
    m_completedCard = new StatsOverviewCard(QStringLiteral("总完成数"),
                                            Theme::success(), Theme::primary(), content);
    m_todayCard = new StatsOverviewCard(QStringLiteral("今日完成"),
                                        Theme::neonPink(), Theme::accent(), content);
    m_streakCard = new StatsOverviewCard(QStringLiteral("连续完成 · 天"),
                                         Theme::warning(), Theme::danger(), content);
    overviewLayout->addWidget(m_totalCard, 1);
    overviewLayout->addWidget(m_completedCard, 1);
    overviewLayout->addWidget(m_todayCard, 1);
    overviewLayout->addWidget(m_streakCard, 1);
    mainLayout->addLayout(overviewLayout);

    // 第二行：完成率环 + 近 14 天柱状图
    QHBoxLayout *midLayout = new QHBoxLayout();
    midLayout->setSpacing(kCardSpacing);
    m_ringCard = new StatsRingCard(content);
    m_ringCard->setFixedWidth(300);
    m_barChart = new StatsBarChart(content);
    midLayout->addWidget(m_ringCard);
    midLayout->addWidget(m_barChart, 1);
    mainLayout->addLayout(midLayout);

    // 第三行：热力图
    m_heatmap = new StatsHeatmap(content);
    mainLayout->addWidget(m_heatmap);

    mainLayout->addStretch();

    scrollArea->setWidget(content);
    outerLayout->addWidget(scrollArea);
}

void StatsWidget::setData(const QList<TodoFolder> &folders)
{
    m_folders = folders;

    int total = 0;
    int completed = 0;
    int todayCompleted = 0;

    const QDate today = QDate::currentDate();
    QVector<QDate> dates(kChartDays);
    QVector<int> dailyCounts(kChartDays, 0);
    QHash<QDate, int> heatMap;

    for (int i = 0; i < kChartDays; ++i)
        dates[i] = today.addDays(i - (kChartDays - 1));   // 最旧的一天在最左

    for (const TodoFolder &folder : m_folders) {
        total += folder.getItemCount();
        completed += folder.getCompletedCount();
        for (const TodoItem &item : folder.getItems()) {
            if (!item.isCompleted() || !item.getCompletedTime().isValid())
                continue;
            const QDate doneDate = item.getCompletedTime().date();
            if (doneDate == today)
                ++todayCompleted;
            const int daysAgo = today.toJulianDay() - doneDate.toJulianDay();
            if (daysAgo >= 0 && daysAgo < kChartDays)
                ++dailyCounts[kChartDays - 1 - daysAgo];
            if (daysAgo >= 0 && daysAgo < kHeatmapDays)
                ++heatMap[doneDate];
        }
    }

    // 连续完成天数：从今天往前数，今天没有则从昨天开始数
    int streak = 0;
    int offset = heatMap.value(today, 0) > 0 ? 0 : 1;
    while (heatMap.value(today.addDays(-(streak + offset)), 0) > 0)
        ++streak;

    m_totalCard->setValue(total);
    m_completedCard->setValue(completed);
    m_todayCard->setValue(todayCompleted);
    m_streakCard->setValue(streak);
    m_ringCard->setRate(completed, total);
    m_barChart->setDailyCounts(dates, dailyCounts);
    m_heatmap->setDailyMap(heatMap);
    update();
}
