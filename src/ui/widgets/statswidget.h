#ifndef STATSWIDGET_H
#define STATSWIDGET_H

#include <QWidget>
#include <QList>
#include <QVector>
#include <QHash>
#include <QDate>
#include <QColor>
#include <QString>
#include <QVariantAnimation>
#include "todofolder.h"

class QEnterEvent;

// 悬浮上浮动效状态（每个卡片一份）
struct HoverState {
    qreal value = 0.0;
    QVariantAnimation *anim = nullptr;
    void attach(QWidget *w);        // 在卡片构造函数中调用
    void start(qreal target);       // 进入/离开时调用
    void enter(QEnterEvent *e);
    void leave(QEvent *e);
};

// 概览小卡片：玻璃拟态底 + 霓虹渐变大号数字（切换时数字滚动动画）
class StatsOverviewCard : public QWidget
{
    Q_OBJECT

public:
    explicit StatsOverviewCard(const QString &caption,
                               const QColor &gradFrom, const QColor &gradTo,
                               QWidget *parent = nullptr);
    void setValue(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override { m_hover.enter(event); }
    void leaveEvent(QEvent *event) override { m_hover.leave(event); }

private:
    QString m_caption;
    QColor m_gradFrom;
    QColor m_gradTo;
    int m_value = 0;
    qreal m_displayValue = 0.0;          // 动画中的当前显示值
    QVariantAnimation *m_anim = nullptr;
    HoverState m_hover;
};

// 完成率环形进度卡：霓虹锥形渐变圆弧 + 中心百分比（入场扫动动画）
class StatsRingCard : public QWidget
{
    Q_OBJECT

public:
    explicit StatsRingCard(QWidget *parent = nullptr);
    void setRate(int completed, int total);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override { m_hover.enter(event); }
    void leaveEvent(QEvent *event) override { m_hover.leave(event); }

private:
    int m_completed = 0;
    int m_total = 0;
    qreal m_progress = 0.0;              // 动画中的当前进度（0~1）
    QVariantAnimation *m_anim = nullptr;
    HoverState m_hover;
};

// 近 14 天完成柱状图：青->紫渐变柱 + 顶部发光 + 入场生长动画
class StatsBarChart : public QWidget
{
    Q_OBJECT

public:
    explicit StatsBarChart(QWidget *parent = nullptr);
    void setDailyCounts(const QVector<QDate> &dates, const QVector<int> &counts);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override { m_hover.enter(event); }
    void leaveEvent(QEvent *event) override { m_hover.leave(event); }

private:
    QVector<QDate> m_dates;
    QVector<int> m_counts;
    qreal m_animProgress = 1.0;          // 入场动画进度（0~1）
    QVariantAnimation *m_anim = nullptr;
    HoverState m_hover;
};

// 完成热力图：GitHub 风格霓虹色阶方格（列 = 周，周一开头对齐，含星期/月份标签与悬停提示）
class StatsHeatmap : public QWidget
{
    Q_OBJECT

public:
    explicit StatsHeatmap(QWidget *parent = nullptr);
    void setDailyMap(const QHash<QDate, int> &map);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override { m_hover.enter(event); }
    void leaveEvent(QEvent *event) override;

private:
    // 网格布局参数（paint/mouse 共用）：列数、网格起点、单元格边距
    struct GridLayout {
        int cols;
        int cell;
        int gap;
        int gridX;      // 网格左上角 x（星期标签右侧）
        int gridY;      // 网格左上角 y（月份标签下方）
        QDate firstDay; // 第一列周一的日期
    };
    GridLayout gridLayout() const;
    QDate dateAt(const QPoint &pos) const;   // 命中检测：坐标 -> 日期（无效 = 未命中）

    QHash<QDate, int> m_map;
    HoverState m_hover;
    QDate m_hoveredDate;                     // 无效日期 = 无悬停
};

// 统计页面：概览卡片 + 完成率环 + 渐变柱状图 + 热力图
class StatsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatsWidget(QWidget *parent = nullptr);
    void setData(const QList<TodoFolder> &folders);  // 由主窗口在数据变化时调用

private:
    void setupUI();

    QList<TodoFolder> m_folders;
    StatsOverviewCard *m_totalCard;
    StatsOverviewCard *m_completedCard;
    StatsOverviewCard *m_todayCard;
    StatsOverviewCard *m_streakCard;
    StatsRingCard *m_ringCard;
    StatsBarChart *m_barChart;
    StatsHeatmap *m_heatmap;
};

#endif // STATSWIDGET_H
