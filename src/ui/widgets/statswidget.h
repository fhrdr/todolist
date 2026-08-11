#ifndef STATSWIDGET_H
#define STATSWIDGET_H

#include <QWidget>
#include <QList>
#include <QVector>
#include <QDate>
#include <QColor>
#include <QString>
#include "todofolder.h"

// 概览小卡片：主题色大号数字 + 下方说明文字
class StatsOverviewCard : public QWidget
{
    Q_OBJECT

public:
    explicit StatsOverviewCard(const QString &caption, const QColor &accentColor, QWidget *parent = nullptr);
    void setValue(int value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_caption;
    QColor m_accentColor;
    int m_value;
};

// 近 14 天完成柱状图（QPainter 自绘，含卡片容器与标题）
class StatsBarChart : public QWidget
{
    Q_OBJECT

public:
    explicit StatsBarChart(QWidget *parent = nullptr);
    void setDailyCounts(const QVector<QDate> &dates, const QVector<int> &counts);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QDate> m_dates;
    QVector<int> m_counts;
};

// 各文件夹完成率横向条形图（QPainter 自绘，含卡片容器与标题）
class StatsFolderChart : public QWidget
{
    Q_OBJECT

public:
    explicit StatsFolderChart(QWidget *parent = nullptr);
    void setFolders(const QList<TodoFolder> &folders);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct FolderStat {
        QString name;
        int completed;
        int total;
    };
    QList<FolderStat> m_stats;   // 按事项总数降序，最多 8 条
};

// 统计页面：概览卡片 + 近 14 天完成柱状图 + 各文件夹完成率
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
    StatsBarChart *m_barChart;
    StatsFolderChart *m_folderChart;
};

#endif // STATSWIDGET_H
