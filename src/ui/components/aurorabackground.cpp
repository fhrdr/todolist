#include "aurorabackground.h"
#include "../theme.h"

#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QTimer>
#include <QResizeEvent>
#include <QRandomGenerator>
#include <cmath>

namespace {
constexpr qreal kConnectDist = 120.0;    // 粒子间连线距离
constexpr qreal kConnectDist2 = kConnectDist * kConnectDist;
constexpr int   kTimerMs     = 50;       // ~20fps 恒定（纯漂浮动效足够流畅且省电）
constexpr int   kCacheMs     = 400;      // 底图缓存最低刷新间隔（光晕漂移极慢，低频即可）
} // namespace

AuroraBackground::AuroraBackground(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);

    rebuildScene();
    m_clock.start();

    m_timer = new QTimer(this);
    m_timer->setInterval(kTimerMs);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        // 仅窗口最小化/隐藏时暂停（不可见时渲染纯属浪费）；
        // 失焦不再降帧，保持正常匀速运行
        QWidget *win = window();
        if (!isVisible() || (win && win->isMinimized())) {
            m_lastTick = m_clock.elapsed();
            return;
        }

        const qint64 now = m_clock.elapsed();
        const qreal dt = qMin<qreal>((now - m_lastTick) / 1000.0, 0.1);
        m_lastTick = now;

        if (m_animated) advance(dt);
        // 光晕漂移极慢，底图缓存低频刷新即可
        if (now - m_lastCacheMs > kCacheMs) {
            m_cacheDirty = true;
        }
        update();
    });
    m_timer->start();
}

void AuroraBackground::setAnimated(bool on)
{
    m_animated = on;
}

QColor AuroraBackground::particleColor(int idx) const
{
    // 霓虹四色循环：青 / 紫 / 粉 / 绿
    static const QColor darkColors[] = {
        QColor(0x22, 0xD3, 0xEE), QColor(0xA8, 0x55, 0xF7),
        QColor(0xF4, 0x72, 0xB6), QColor(0x34, 0xD3, 0x99)
    };
    static const QColor lightColors[] = {
        QColor(0x4F, 0x46, 0xE5), QColor(0x8B, 0x5C, 0xF6),
        QColor(0xEC, 0x48, 0x99), QColor(0x10, 0xB9, 0x81)
    };
    const auto &table = Theme::isDark() ? darkColors : lightColors;
    return table[idx % 4];
}

bool AuroraBackground::event(QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        rebuildScene();
        m_cacheDirty = true;
    }
    return QWidget::event(event);
}

void AuroraBackground::rebuildScene()
{
    m_blobs.clear();
    m_particles.clear();

    auto *rng = QRandomGenerator::global();
    const bool dark = Theme::isDark();

    // 两三团极淡光晕做纵深（缓慢漂移）；浅色模式用更柔和的 tint
    const int orbAlpha = dark ? 34 : 52;
    m_blobs.append({ dark ? QColor(0x22, 0xD3, 0xEE, orbAlpha) : QColor(0xA5, 0xF3, 0xFC, orbAlpha), 0.16, 0.20, 0.55,
                     rng->generateDouble() * 6.28, rng->generateDouble() * 6.28,
                     0.00010, 0.00008, 0.08, 0.06 });
    m_blobs.append({ dark ? QColor(0xA8, 0x55, 0xF7, orbAlpha) : QColor(0xC4, 0xB5, 0xFD, orbAlpha), 0.86, 0.30, 0.60,
                     rng->generateDouble() * 6.28, rng->generateDouble() * 6.28,
                     0.00007, 0.00011, 0.06, 0.09 });
    m_blobs.append({ dark ? QColor(0xF4, 0x72, 0xB6, 24) : QColor(0xF9, 0xA8, 0xD4, 44), 0.55, 0.96, 0.62,
                     rng->generateDouble() * 6.28, rng->generateDouble() * 6.28,
                     0.00009, 0.00006, 0.10, 0.05 });

    // 粒子数量随面积自适应（上限收紧，省电）
    const int count = qBound(24, width() * height() / 20000, 64);
    for (int i = 0; i < count; ++i) {
        Particle pt;
        pt.x = rng->generateDouble() * qMax(width(), 1);
        pt.y = rng->generateDouble() * qMax(height(), 1);
        // 速度：14~34 px/s，随机方向
        const qreal angle = rng->generateDouble() * 2 * M_PI;
        const qreal speed = 14.0 + rng->generateDouble() * 20.0;
        pt.vx = std::cos(angle) * speed;
        pt.vy = std::sin(angle) * speed;
        pt.size = 1.4 + rng->generateDouble() * 1.8;
        pt.colorIdx = i;
        m_particles.append(pt);
    }
}

void AuroraBackground::advance(qreal dt)
{
    const qreal W = qMax(width(), 1);
    const qreal H = qMax(height(), 1);

    for (Particle &pt : m_particles) {
        pt.x += pt.vx * dt;
        pt.y += pt.vy * dt;

        // 环绕边界：从一侧出去，另一侧回来
        if (pt.x < -20)      pt.x = W + 20;
        else if (pt.x > W + 20) pt.x = -20;
        if (pt.y < -20)      pt.y = H + 20;
        else if (pt.y > H + 20) pt.y = -20;
    }
}

// 底色 + 远景光晕渲染进半分辨率离屏缓存：径向渐变是全帧最贵操作，
// 光晕漂移周期以分钟计，低频重绘 + 每帧 blit 几乎无视觉差异
void AuroraBackground::rebuildBaseCache()
{
    const int W = qMax(width() / 2, 1);
    const int H = qMax(height() / 2, 1);
    const bool dark = Theme::isDark();
    const qreal t = static_cast<qreal>(m_clock.elapsed());
    const qreal shortSide = qMin(W, H);

    m_baseCache = QPixmap(W, H);
    m_baseCache.fill(Qt::transparent);

    QPainter p(&m_baseCache);
    p.setRenderHint(QPainter::Antialiasing);

    // ---- 底色：深空渐变 ----
    QLinearGradient base(0, 0, 0, H);
    if (dark) {
        base.setColorAt(0.0, QColor(0x0B, 0x0E, 0x1A));
        base.setColorAt(0.55, QColor(0x0E, 0x13, 0x26));
        base.setColorAt(1.0, QColor(0x0A, 0x0C, 0x18));
    } else {
        // 浅色：柔和蓝灰渐变底（不再惨白，与白色玻璃卡拉开层次）
        base.setColorAt(0.0, QColor(0xE7, 0xEC, 0xF6));
        base.setColorAt(0.55, QColor(0xE0, 0xE7, 0xF3));
        base.setColorAt(1.0, QColor(0xE9, 0xE5, 0xF2));
    }
    p.fillRect(QRect(0, 0, W, H), base);

    // ---- 远景光晕 ----
    p.setCompositionMode(QPainter::CompositionMode_Screen);
    for (const Blob &b : m_blobs) {
        const qreal cx = (b.baseX + b.ampX * std::sin(t * b.speedX + b.phaseX)) * W;
        const qreal cy = (b.baseY + b.ampY * std::cos(t * b.speedY + b.phaseY)) * H;
        const qreal r  = b.radius * shortSide;

        QRadialGradient g(QPointF(cx, cy), r);
        QColor c0 = b.color;
        QColor c1 = b.color;
        c1.setAlpha(0);
        g.setColorAt(0.0, c0);
        g.setColorAt(1.0, c1);
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawEllipse(QPointF(cx, cy), r, r);
    }
    p.end();

    m_lastCacheMs = m_clock.elapsed();
    m_cacheDirty = false;
}

void AuroraBackground::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_cacheDirty || m_baseCache.isNull()) {
        rebuildBaseCache();
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const bool dark = Theme::isDark();

    // ---- 底图（半分辨率缓存放大 blit，模糊感对光晕无损） ----
    p.drawPixmap(rect(), m_baseCache);

    // ---- 粒子连线（全程平方距离比较，命中才开方） ----
    const int n = m_particles.size();
    const qreal maxLineAlpha = dark ? 110.0 : 100.0;
    for (int i = 0; i < n; ++i) {
        const Particle &a = m_particles[i];
        for (int j = i + 1; j < n; ++j) {
            const Particle &b = m_particles[j];
            const qreal dx = a.x - b.x;
            if (std::abs(dx) > kConnectDist) continue;
            const qreal dy = a.y - b.y;
            if (std::abs(dy) > kConnectDist) continue;
            const qreal d2 = dx * dx + dy * dy;
            if (d2 > kConnectDist2) continue;

            const qreal closeness = 1.0 - d2 / kConnectDist2;   // 平方近似，视觉足够
            QColor c = particleColor(a.colorIdx);
            c.setAlpha(static_cast<int>(maxLineAlpha * closeness));
            p.setPen(QPen(c, 1));
            p.drawLine(QPointF(a.x, a.y), QPointF(b.x, b.y));
        }
    }

    // ---- 粒子点（带微光晕；浅色模式提高不透明度保证可见） ----
    for (const Particle &pt : m_particles) {
        QColor c = particleColor(pt.colorIdx);
        QColor glow = c;
        glow.setAlpha(dark ? 40 : 48);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size * 2.6, pt.size * 2.6);
        c.setAlpha(dark ? 210 : 235);
        p.setBrush(c);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size, pt.size);
    }
}
