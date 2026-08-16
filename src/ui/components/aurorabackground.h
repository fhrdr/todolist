#ifndef AURORABACKGROUND_H
#define AURORABACKGROUND_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QPointF>
#include <QPixmap>
#include <QElapsedTimer>

// 粒子动效背景：深空渐变底 + 漂浮粒子 + 邻近粒子霓虹连线
// 性能设计：底色/光晕渲染进半分辨率离屏缓存（低频刷新），
// 每帧只 blit 缓存 + 画粒子；恒定 ~20fps，最小化时暂停，无鼠标交互（省电）
class AuroraBackground : public QWidget
{
    Q_OBJECT

public:
    explicit AuroraBackground(QWidget *parent = nullptr);

    void setAnimated(bool on);   // 关闭后粒子静止（省电/低配）

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    struct Particle {
        qreal x, y;       // 像素坐标
        qreal vx, vy;     // 像素/秒
        qreal size;
        int colorIdx;     // 霓虹色索引
    };
    struct Blob {
        QColor color;
        qreal baseX, baseY;      // 归一化中心位置（0~1）
        qreal radius;            // 归一化半径（相对短边）
        qreal phaseX, phaseY;    // 漂移相位
        qreal speedX, speedY;    // 漂移速度
        qreal ampX, ampY;        // 漂移幅度
    };

    void rebuildScene();
    void rebuildBaseCache();     // 重绘底色 + 光晕离屏缓存
    void advance(qreal dt);      // 推进粒子位置
    QColor particleColor(int idx) const;

    QVector<Particle> m_particles;
    QVector<Blob> m_blobs;
    QElapsedTimer m_clock;
    qint64 m_lastTick = 0;      // 上一帧时间戳（毫秒），用于计算 dt
    QTimer *m_timer = nullptr;
    bool m_animated = true;

    QPixmap m_baseCache;        // 底色 + 光晕半分辨率缓存
    qint64 m_lastCacheMs = -10000;
    bool m_cacheDirty = true;
};

#endif // AURORABACKGROUND_H
