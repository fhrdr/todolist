#include "statswidget.h"
#include "../theme.h"
#include "../components/sectionheader.h"

#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <algorithm>

namespace {
constexpr int kCardPadding = 20;   // 卡片内边距
constexpr int kCardSpacing = 14;   // 卡片间距
constexpr int kPageMargin  = 20;   // 页面外边距
constexpr int kChartDays   = 14;   // 柱状图统计天数
constexpr int kMaxFolders  = 8;    // 条形图最多显示的文件夹数

// 统一的卡片容器：surface 背景 + 1px border + radiusLg 圆角
void drawCardBackground(QPainter &painter, const QRect &rect)
{
    painter.setPen(QPen(Theme::border(), 1));
    painter.setBrush(Theme::surface());
    painter.drawRoundedRect(rect.adjusted(0, 0, -1, -1), Theme::radiusLg, Theme::radiusLg);
}

void drawCardTitle(QPainter &painter, const QString &title, const QRect &rect)
{
    painter.setFont(Theme::font(Theme::fontMedium, QFont::DemiBold));
    painter.setPen(Theme::textPrimary());
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, title);
}
} // namespace

// ---------------- StatsOverviewCard ----------------

StatsOverviewCard::StatsOverviewCard(const QString &caption, const QColor &accentColor, QWidget *parent)
    : QWidget(parent)
    , m_caption(caption)
    , m_accentColor(accentColor)
    , m_value(0)
{
    setFixedHeight(90);
}

void StatsOverviewCard::setValue(int value)
{
    m_value = value;
    update();
}

void StatsOverviewCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawCardBackground(painter, rect());

    painter.setFont(Theme::font(Theme::fontHero + 8, QFont::Bold));
    painter.setPen(m_accentColor);
    painter.drawText(QRect(0, 12, width(), 40), Qt::AlignHCenter | Qt::AlignVCenter,
                     QString::number(m_value));

    painter.setFont(Theme::font(Theme::fontSmall));
    painter.setPen(Theme::textSecondary());
    painter.drawText(QRect(0, 54, width(), 20), Qt::AlignHCenter | Qt::AlignVCenter, m_caption);
}

// ---------------- StatsBarChart ----------------

StatsBarChart::StatsBarChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(220);
}

void StatsBarChart::setDailyCounts(const QVector<QDate> &dates, const QVector<int> &counts)
{
    m_dates = dates;
    m_counts = counts;
    update();
}

void StatsBarChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawCardBackground(painter, rect());

    const int pad = kCardPadding;
    const QRect titleRect(pad, pad, width() - 2 * pad, 22);
    drawCardTitle(painter, QStringLiteral("近 14 天完成"), titleRect);

    // 绘图区：左侧留 36px 给 Y 轴数值标签，底部留 22px 给 X 轴日期标签
    QRect plotRect(pad + 36, titleRect.bottom() + 14, width() - 2 * pad - 36, 0);
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

    // Y 轴：最多 4 条淡色网格线，刻度为 maxCount 向上取整到 4 的倍数
    const int step = (maxCount + 3) / 4;
    const int yMax = step * 4;

    painter.setFont(Theme::font(Theme::fontSmall));
    for (int i = 0; i <= 4; ++i) {
        const int value = step * i;
        const qreal y = plotRect.bottom() - (qreal)value / yMax * plotRect.height();
        painter.setPen(QPen(Theme::border(), 1));
        painter.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
        if (value > 0) {   // 基线不画数值标签
            painter.setPen(Theme::textMuted());
            painter.drawText(QRect(pad, (int)y - 8, plotRect.left() - 6 - pad, 16),
                             Qt::AlignRight | Qt::AlignVCenter, QString::number(value));
        }
    }

    const qreal slotW = (qreal)plotRect.width() / days;
    const qreal barW = qMin<qreal>(slotW * 0.55, 36.0);

    for (int i = 0; i < days; ++i) {
        const int count = m_counts.at(i);
        const qreal x = plotRect.left() + i * slotW + (slotW - barW) / 2.0;
        const qreal bottom = plotRect.bottom();

        if (count > 0) {
            qreal barH = (qreal)count / yMax * plotRect.height();
            barH = qMax<qreal>(barH, 3.0);
            const qreal y = bottom - barH;
            const qreal r = qMin<qreal>(4.0, qMin(barW, barH) / 2.0);

            // 仅顶部圆角的柱子
            QPainterPath barPath;
            barPath.moveTo(x, bottom);
            barPath.lineTo(x, y + r);
            barPath.quadTo(x, y, x + r, y);
            barPath.lineTo(x + barW - r, y);
            barPath.quadTo(x + barW, y, x + barW, y + r);
            barPath.lineTo(x + barW, bottom);
            barPath.closeSubpath();

            painter.setPen(Qt::NoPen);
            painter.setBrush(Theme::primary());
            painter.drawPath(barPath);

            painter.setFont(Theme::font(Theme::fontSmall));
            painter.setPen(Theme::textSecondary());
            painter.drawText(QRectF(x + barW / 2.0 - slotW / 2.0, y - 18, slotW, 16),
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

// ---------------- StatsFolderChart ----------------

StatsFolderChart::StatsFolderChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(220);
}

void StatsFolderChart::setFolders(const QList<TodoFolder> &folders)
{
    m_stats.clear();
    for (const TodoFolder &folder : folders) {
        FolderStat stat;
        stat.name = folder.getName();
        stat.total = folder.getItemCount();
        stat.completed = folder.getCompletedCount();
        m_stats.append(stat);
    }
    std::sort(m_stats.begin(), m_stats.end(), [](const FolderStat &a, const FolderStat &b) {
        return a.total > b.total;
    });
    if (m_stats.size() > kMaxFolders)
        m_stats = m_stats.mid(0, kMaxFolders);
    update();
}

void StatsFolderChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawCardBackground(painter, rect());

    const int pad = kCardPadding;
    const QRect titleRect(pad, pad, width() - 2 * pad, 22);
    drawCardTitle(painter, QStringLiteral("各文件夹完成率"), titleRect);

    const QRect contentRect(pad, titleRect.bottom() + 12,
                            width() - 2 * pad, height() - pad - titleRect.bottom() - 12);

    if (m_stats.isEmpty()) {
        painter.setFont(Theme::font(Theme::fontBase));
        painter.setPen(Theme::textMuted());
        painter.drawText(contentRect, Qt::AlignCenter, QStringLiteral("暂无文件夹数据"));
        return;
    }

    const int nameW = 120;    // 左侧文件夹名列宽
    const int valueW = 110;   // 右侧 "3/10 · 30%" 列宽
    const int barH = 14;
    const qreal rowH = qMin<qreal>(30.0, (qreal)contentRect.height() / m_stats.size());
    const QFontMetrics nameFm(Theme::font(Theme::fontBase));

    for (int i = 0; i < m_stats.size(); ++i) {
        const FolderStat &stat = m_stats.at(i);
        const qreal rowY = contentRect.top() + i * rowH;
        const qreal centerY = rowY + rowH / 2.0;

        painter.setFont(Theme::font(Theme::fontBase));
        painter.setPen(Theme::textPrimary());
        const QString elidedName = nameFm.elidedText(stat.name, Qt::ElideRight, nameW);
        painter.drawText(QRectF(contentRect.left(), rowY, nameW, rowH),
                         Qt::AlignLeft | Qt::AlignVCenter, elidedName);

        const qreal barX = contentRect.left() + nameW + 12;
        const qreal barMaxW = contentRect.right() - valueW - 12 - barX;
        const qreal barY = centerY - barH / 2.0;

        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::surfaceAlt());
        painter.drawRoundedRect(QRectF(barX, barY, barMaxW, barH), 7, 7);

        if (stat.total > 0 && stat.completed > 0) {
            qreal fillW = barMaxW * stat.completed / stat.total;
            fillW = qMin(barMaxW, qMax<qreal>(fillW, 7.0));
            painter.setBrush(Theme::success());
            painter.drawRoundedRect(QRectF(barX, barY, fillW, barH), 7, 7);
        }

        const int percent = stat.total > 0 ? stat.completed * 100 / stat.total : 0;
        const QString valueText = QStringLiteral("%1/%2 · %3%")
                                      .arg(stat.completed).arg(stat.total).arg(percent);
        painter.setFont(Theme::font(Theme::fontSmall));
        painter.setPen(Theme::textSecondary());
        painter.drawText(QRectF(contentRect.right() - valueW, rowY, valueW, rowH),
                         Qt::AlignRight | Qt::AlignVCenter, valueText);
    }
}

// ---------------- StatsWidget ----------------

StatsWidget::StatsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void StatsWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(kPageMargin, kPageMargin, kPageMargin, kPageMargin);
    mainLayout->setSpacing(kCardSpacing);

    SectionHeader *header = new SectionHeader(QStringLiteral("统计"), this);
    mainLayout->addWidget(header);

    QHBoxLayout *overviewLayout = new QHBoxLayout();
    overviewLayout->setSpacing(kCardSpacing);
    m_totalCard = new StatsOverviewCard(QStringLiteral("总待办数"), Theme::primary(), this);
    m_completedCard = new StatsOverviewCard(QStringLiteral("总完成数"), Theme::success(), this);
    m_todayCard = new StatsOverviewCard(QStringLiteral("今日完成数"), Theme::accent(), this);
    overviewLayout->addWidget(m_totalCard, 1);
    overviewLayout->addWidget(m_completedCard, 1);
    overviewLayout->addWidget(m_todayCard, 1);
    mainLayout->addLayout(overviewLayout);

    m_barChart = new StatsBarChart(this);
    mainLayout->addWidget(m_barChart, 1);

    m_folderChart = new StatsFolderChart(this);
    mainLayout->addWidget(m_folderChart, 1);
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
        }
    }

    m_totalCard->setValue(total);
    m_completedCard->setValue(completed);
    m_todayCard->setValue(todayCompleted);
    m_barChart->setDailyCounts(dates, dailyCounts);
    m_folderChart->setFolders(m_folders);
    update();
}
