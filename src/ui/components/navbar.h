#ifndef NAVBAR_H
#define NAVBAR_H

#include <QWidget>
#include <QPushButton>
#include <QStringList>
#include <QStackedWidget>
#include <QVariantAnimation>
#include <QPixmap>
#include <QPointer>
#include "../icons.h"

// 顶部导航栏：左侧应用标识，居中导航项（点击切换页面），
// 选中项为霓虹描边胶囊指示器，切换时胶囊平滑滑动。
// 配合 attachStack() 可实现页面内容淡入淡出过渡（背景保持不变）。
class NavBar : public QWidget
{
    Q_OBJECT

public:
    explicit NavBar(const QString &appName, const QStringList &items, QWidget *parent = nullptr);

    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }

    // 关联页面容器，切换时带内容淡入淡出过渡
    void attachStack(QStackedWidget *stack);

    void addRightWidget(QWidget *w);

signals:
    void pageSelected(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRect itemRect(int index) const;
    QRectF capsuleRect(int index) const;
    void moveIndicator(int index, bool animated);
    void slideToPage(int index);

    QString m_appName;
    QStringList m_items;
    int m_currentIndex = 0;
    int m_hoveredIndex = -1;

    QRectF m_indRect;                       // 选中胶囊当前位置（动画驱动）
    QVariantAnimation *m_indAnim = nullptr;
    QStackedWidget *m_stack = nullptr;

    QPointer<QWidget> m_fadeOverlay;        // 进行中的过渡层（打断时立即清理）
    QPointer<QWidget> m_fadeHiddenPage;     // 过渡期间被临时隐藏的新页面

    QWidget *m_rightContainer;
    class QHBoxLayout *m_rightLayout;
};

// 导航栏图标按钮：悬停时霓虹光晕渐入 + 图标着色平滑过渡
class NavIconButton : public QPushButton
{
    Q_OBJECT

public:
    explicit NavIconButton(Icons::Type type, const QString &tooltip, QWidget *parent = nullptr);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void animateTo(qreal target, int duration);

    QPixmap m_pmNormal;    // 常态图标（灰）
    QPixmap m_pmHover;     // 悬停图标（主题色）
    qreal m_hover = 0.0;
    QVariantAnimation *m_anim = nullptr;
};

#endif // NAVBAR_H
