#ifndef AURORABACKGROUND_H
#define AURORABACKGROUND_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QPointF>
#include <QPixmap>
#include <QElapsedTimer>

// 粒子动效背景：深空渐变底 + 多种粒子动效（星点连线/萤火/气泡/雪花/流星）
// 性能设计：底色/光晕渲染进半分辨率离屏缓存（低频刷新），
// 每帧只 blit 缓存 + 画粒子；恒定 ~20fps，最小化时暂停，无鼠标交互（省电）
class AuroraBackground : public QWidget
{
    Q_OBJECT

public:
    // 背景粒子动效类型
    enum Effect {
        Constellation = 0,   // 星点连线（经典 plexus）
        Fireflies,           // 萤火流光（缓慢游弋 + 呼吸明灭）
        Bubbles,             // 气泡上升（摇摆上浮 + 圆环）
        Snowfall,            // 雪花飘落（左右摇摆下落）
        Meteors,             // 流星划过（繁星闪烁 + 拖尾流星）
        EffectCount
    };

    explicit AuroraBackground(QWidget *parent = nullptr);

    void setAnimated(bool on);      // 关闭后粒子静止（省电/低配）
    void setEffect(int effect);     // 切换粒子动效（重建粒子场景）
    int effect() const { return m_effect; }

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    struct Particle {
        qreal x, y;       // 像素坐标
        qreal vx, vy;     // 像素/秒
        qreal size;
        int colorIdx;     // 霓虹色索引
        qreal phase;      // 呼吸/摇摆相位
        qreal life;       // 流星剩余生命（秒）；<=0 表示待生成
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
    void advance(qreal dt);      // 推进粒子位置（按当前动效）
    QColor particleColor(int idx) const;

    // 各动效的每帧绘制
    void paintConstellation(QPainter &p);
    void paintFireflies(QPainter &p);
    void paintBubbles(QPainter &p);
    void paintSnowfall(QPainter &p);
    void paintMeteors(QPainter &p);

    QVector<Particle> m_particles;
    QVector<Blob> m_blobs;
    QElapsedTimer m_clock;
    qint64 m_lastTick = 0;      // 上一帧时间戳（毫秒），用于计算 dt
    QTimer *m_timer = nullptr;
    bool m_animated = true;
    int m_effect = Constellation;

    QPixmap m_baseCache;        // 底色 + 光晕半分辨率缓存
    qint64 m_lastCacheMs = -10000;
    bool m_cacheDirty = true;
};

#endif // AURORABACKGROUND_H
