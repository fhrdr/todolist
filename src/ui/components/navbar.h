#ifndef NAVBAR_H
#define NAVBAR_H

#include <QWidget>
#include <QStringList>
#include <QStackedWidget>
#include <QPropertyAnimation>

// 顶部导航栏：左侧应用标识，居中导航项（点击切换页面），
// 选中项下方有主题色下划线，切换时下划线平滑滑动。
// 配合 attachStack() 可实现页面淡入淡出切换。
class NavBar : public QWidget
{
    Q_OBJECT

public:
    explicit NavBar(const QString &appName, const QStringList &items, QWidget *parent = nullptr);

    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }

    // 关联页面容器，切换时带淡入动画
    void attachStack(QStackedWidget *stack);

    void addRightWidget(QWidget *w);

signals:
    void pageSelected(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QRect itemRect(int index) const;
    void moveIndicator(int index, bool animated);
    void fadeToPage(int index);

    QString m_appName;
    QStringList m_items;
    int m_currentIndex = 0;
    int m_hoveredIndex = -1;

    QWidget *m_indicator;
    QPropertyAnimation *m_indicatorAnim = nullptr;
    QStackedWidget *m_stack = nullptr;

    QWidget *m_rightContainer;
    class QHBoxLayout *m_rightLayout;
};

#endif // NAVBAR_H
