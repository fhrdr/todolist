#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QPoint>

// 自定义无边框标题栏：
// 左侧应用图标 + 标题，右侧最小化 / 最大化(还原) / 关闭按钮
// 空白区域拖动移动窗口，双击切换最大化
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(const QString &title, QWidget *parent = nullptr);

    void setTitle(const QString &title);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    enum Button { BtnNone = -1, BtnMin = 0, BtnMax = 1, BtnClose = 2 };

    QRect buttonRect(Button btn) const;
    Button buttonAt(const QPoint &pos) const;
    void toggleMaxRestore();

    QString m_title;
    int m_hoverBtn = BtnNone;    // 当前悬停的按钮
    int m_pressedBtn = BtnNone;  // 当前按下的按钮

    bool m_dragging = false;
    QPoint m_dragOffset;         // 按下点相对窗口左上角偏移
};

#endif // TITLEBAR_H
