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
constexpr qreal kConnectDist = 120.0;    // 星点连线距离
constexpr qreal kConnectDist2 = kConnectDist * kConnectDist;
constexpr int   kTimerMs     = 50;       // ~20fps 恒定（纯漂浮动效足够流畅且省电）
constexpr int   kCacheMs     = 400;      // 底图缓存最低刷新间隔（光晕漂移极慢，低频即可）

inline qreal randDouble(qreal lo, qreal hi)
{
    return lo + QRandomGenerator::global()->generateDouble() * (hi - lo);
}
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

void AuroraBackground::setEffect(int effect)
{
    if (effect < 0 || effect >= EffectCount || effect == m_effect) {
        return;
    }
    m_effect = effect;
    rebuildScene();
    update();
}

QColor AuroraBackground::particleColor(int idx) const
{
    // 霓虹四色循环：青 / 紫 / 粉 / 绿（浅色模式换用更深饱和的配色保证可见）
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
    const qreal W = qMax(width(), 1);
    const qreal H = qMax(height(), 1);

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
    const int area = width() * height();
    switch (m_effect) {
    case Fireflies: {
        const int count = qBound(16, area / 32000, 36);
        for (int i = 0; i < count; ++i) {
            Particle pt;
            pt.x = randDouble(0, W);
            pt.y = randDouble(0, H);
            const qreal angle = randDouble(0, 2 * M_PI);
            const qreal speed = randDouble(6.0, 16.0);
            pt.vx = std::cos(angle) * speed;
            pt.vy = std::sin(angle) * speed;
            pt.size = randDouble(1.6, 3.0);
            pt.colorIdx = i;
            pt.phase = randDouble(0, 6.28);
            pt.life = 0;
            m_particles.append(pt);
        }
        break;
    }
    case Bubbles: {
        const int count = qBound(18, area / 26000, 46);
        for (int i = 0; i < count; ++i) {
            Particle pt;
            pt.x = randDouble(0, W);
            pt.y = randDouble(0, H);
            pt.vx = randDouble(-6.0, 6.0);
            pt.vy = -randDouble(16.0, 38.0);      // 上浮
            pt.size = randDouble(2.2, 6.0);
            pt.colorIdx = i;
            pt.phase = randDouble(0, 6.28);
            pt.life = 0;
            m_particles.append(pt);
        }
        break;
    }
    case Snowfall: {
        const int count = qBound(26, area / 18000, 68);
        for (int i = 0; i < count; ++i) {
            Particle pt;
            pt.x = randDouble(0, W);
            pt.y = randDouble(0, H);
            pt.vx = randDouble(-4.0, 4.0);
            pt.vy = randDouble(22.0, 52.0);       // 下落
            pt.size = randDouble(1.2, 2.8);
            pt.colorIdx = i;
            pt.phase = randDouble(0, 6.28);
            pt.life = 0;
            m_particles.append(pt);
        }
        break;
    }
    case Meteors: {
        // 前景：缓慢漂移的闪烁繁星
        const int starCount = qBound(24, area / 30000, 44);
        for (int i = 0; i < starCount; ++i) {
            Particle pt;
            pt.x = randDouble(0, W);
            pt.y = randDouble(0, H);
            pt.vx = randDouble(-4.0, 4.0);
            pt.vy = randDouble(-3.0, 3.0);
            pt.size = randDouble(0.9, 1.8);
            pt.colorIdx = i;
            pt.phase = randDouble(0, 6.28);
            pt.life = 0;                          // life = 0 → 星星
            m_particles.append(pt);
        }
        // 流星槽位：life < 0 待机，随机触发
        for (int i = 0; i < 5; ++i) {
            Particle pt;
            pt.x = pt.y = -100;
            pt.vx = pt.vy = 0;
            pt.size = randDouble(1.6, 2.4);
            pt.colorIdx = i;
            pt.phase = 0;
            pt.life = -randDouble(0.5, 6.0);      // 负值 = 触发倒计时
            m_particles.append(pt);
        }
        break;
    }
    case Constellation:
    default: {
        const int count = qBound(24, area / 20000, 64);
        for (int i = 0; i < count; ++i) {
            Particle pt;
            pt.x = randDouble(0, W);
            pt.y = randDouble(0, H);
            const qreal angle = randDouble(0, 2 * M_PI);
            const qreal speed = randDouble(14.0, 34.0);
            pt.vx = std::cos(angle) * speed;
            pt.vy = std::sin(angle) * speed;
            pt.size = randDouble(1.4, 3.2);
            pt.colorIdx = i;
            pt.phase = randDouble(0, 6.28);
            pt.life = 0;
            m_particles.append(pt);
        }
        break;
    }
    }
}

void AuroraBackground::advance(qreal dt)
{
    const qreal W = qMax(width(), 1);
    const qreal H = qMax(height(), 1);

    switch (m_effect) {
    case Fireflies:
        for (Particle &pt : m_particles) {
            pt.phase += dt * 1.6;
            // 游弋：速度方向缓慢摆动
            pt.vx += std::sin(pt.phase * 0.9) * 7.0 * dt;
            pt.vy += std::cos(pt.phase * 0.7) * 7.0 * dt;
            const qreal sp2 = pt.vx * pt.vx + pt.vy * pt.vy;
            if (sp2 > 20.0 * 20.0) { const qreal s = 20.0 / std::sqrt(sp2); pt.vx *= s; pt.vy *= s; }
            pt.x += pt.vx * dt;
            pt.y += pt.vy * dt;
            if (pt.x < -20)      pt.x = W + 20;
            else if (pt.x > W + 20) pt.x = -20;
            if (pt.y < -20)      pt.y = H + 20;
            else if (pt.y > H + 20) pt.y = -20;
        }
        break;

    case Bubbles:
        for (Particle &pt : m_particles) {
            pt.phase += dt * 2.2;
            pt.x += (pt.vx + std::sin(pt.phase) * 11.0) * dt;   // 摇摆
            pt.y += pt.vy * dt;
            if (pt.y < -14) {                                    // 浮出水面 → 底部重生
                pt.y = H + 12;
                pt.x = randDouble(0, W);
                pt.vy = -randDouble(16.0, 38.0);
            }
            if (pt.x < -14)      pt.x = W + 12;
            else if (pt.x > W + 14) pt.x = -12;
        }
        break;

    case Snowfall:
        for (Particle &pt : m_particles) {
            pt.phase += dt * 1.7;
            pt.x += (pt.vx + std::sin(pt.phase) * 15.0) * dt;   // 左右飘摆
            pt.y += pt.vy * dt;
            if (pt.y > H + 12) {                                 // 落地 → 顶部重生
                pt.y = -10;
                pt.x = randDouble(0, W);
            }
            if (pt.x < -12)      pt.x = W + 10;
            else if (pt.x > W + 12) pt.x = -10;
        }
        break;

    case Meteors:
        for (Particle &pt : m_particles) {
            if (pt.life < 0) {
                // 待机流星：冷却倒计时结束 → 从顶部随机位置发射
                pt.life += dt;
                if (pt.life >= 0) {
                    pt.x = randDouble(W * 0.15, W * 1.05);
                    pt.y = randDouble(-30.0, H * 0.25);
                    const qreal angle = randDouble(0.55, 0.85) * M_PI;   // 朝左下方
                    const qreal speed = randDouble(300.0, 460.0);
                    pt.vx = std::cos(angle) * speed;
                    pt.vy = std::sin(angle - 0.9) * speed;               // 偏下的飞行角
                    pt.life = randDouble(0.7, 1.3);                      // 飞行时长
                }
                continue;
            }
            if (pt.life > 0) {
                // 飞行中的流星：推进 + 寿命耗尽后进入冷却
                pt.life -= dt;
                pt.x += pt.vx * dt;
                pt.y += pt.vy * dt;
                if (pt.life <= 0) {
                    pt.life = -randDouble(2.5, 7.0);   // 冷却倒计时
                    pt.x = pt.y = -100;
                }
                continue;
            }
            // 繁星（life == 0）：极慢漂移 + 闪烁相位
            pt.phase += dt * 2.4;
            pt.x += pt.vx * dt;
            pt.y += pt.vy * dt;
            if (pt.x < -8)       pt.x = W + 8;
            else if (pt.x > W + 8)  pt.x = -8;
            if (pt.y < -8)       pt.y = H + 8;
            else if (pt.y > H + 8)  pt.y = -8;
        }
        break;

    case Constellation:
    default:
        for (Particle &pt : m_particles) {
            pt.x += pt.vx * dt;
            pt.y += pt.vy * dt;
            // 环绕边界：从一侧出去，另一侧回来
            if (pt.x < -20)      pt.x = W + 20;
            else if (pt.x > W + 20) pt.x = -20;
            if (pt.y < -20)      pt.y = H + 20;
            else if (pt.y > H + 20) pt.y = -20;
        }
        break;
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

// ---- 星点连线：经典 plexus ----
void AuroraBackground::paintConstellation(QPainter &p)
{
    const bool dark = Theme::isDark();
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

// ---- 萤火流光：缓慢游弋 + 呼吸明灭（无连线） ----
void AuroraBackground::paintFireflies(QPainter &p)
{
    const bool dark = Theme::isDark();
    for (const Particle &pt : m_particles) {
        // 呼吸：0.15 ~ 1.0 平滑明灭
        const qreal pulse = 0.15 + 0.85 * (0.5 + 0.5 * std::sin(pt.phase * 2.0));
        QColor c = particleColor(pt.colorIdx);

        QColor halo = c;
        halo.setAlpha(static_cast<int>((dark ? 46 : 56) * pulse));
        p.setPen(Qt::NoPen);
        p.setBrush(halo);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size * 4.2 * pulse + 1.0, pt.size * 4.2 * pulse + 1.0);

        c.setAlpha(static_cast<int>((dark ? 225 : 240) * pulse));
        p.setBrush(c);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size, pt.size);
    }
}

// ---- 气泡上升：圆环 + 内填微光 ----
void AuroraBackground::paintBubbles(QPainter &p)
{
    const bool dark = Theme::isDark();
    for (const Particle &pt : m_particles) {
        QColor c = particleColor(pt.colorIdx);
        const QPointF center(pt.x, pt.y);

        QColor fill = c;
        fill.setAlpha(dark ? 18 : 26);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawEllipse(center, pt.size, pt.size);

        c.setAlpha(dark ? 130 : 165);
        p.setPen(QPen(c, 1.1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, pt.size, pt.size);

        // 高光点：左上一点，气泡感
        QColor hl = c;
        hl.setAlpha(dark ? 200 : 220);
        p.setPen(Qt::NoPen);
        p.setBrush(hl);
        p.drawEllipse(center + QPointF(-pt.size * 0.35, -pt.size * 0.35),
                      pt.size * 0.22, pt.size * 0.22);
    }
}

// ---- 雪花飘落：柔和雪点（浅色模式用蓝灰色，白底可见） ----
void AuroraBackground::paintSnowfall(QPainter &p)
{
    const bool dark = Theme::isDark();
    for (const Particle &pt : m_particles) {
        // 深色：雪白微蓝；浅色： slate 蓝灰（白色在浅底上看不清）
        QColor c = dark ? QColor(0xE0, 0xF2, 0xFE)
                        : Theme::mix(QColor(0x64, 0x74, 0x8B), particleColor(pt.colorIdx), 0.25);
        const int coreAlpha = dark ? 190 : 200;

        QColor glow = c;
        glow.setAlpha(dark ? 34 : 30);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size * 2.4, pt.size * 2.4);

        c.setAlpha(coreAlpha);
        p.setBrush(c);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size, pt.size);
    }
}

// ---- 流星划过：繁星闪烁 + 拖尾流星 ----
void AuroraBackground::paintMeteors(QPainter &p)
{
    const bool dark = Theme::isDark();
    for (const Particle &pt : m_particles) {
        if (pt.life == 0) {
            // 繁星：闪烁小点
            const qreal tw = 0.25 + 0.75 * (0.5 + 0.5 * std::sin(pt.phase * 2.4));
            QColor c = dark ? QColor(0xBA, 0xE6, 0xFD)
                            : Theme::mix(QColor(0x4F, 0x46, 0xE5), QColor(0x0E, 0xA5, 0xE9), 0.4);
            c.setAlpha(static_cast<int>((dark ? 200 : 210) * tw));
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawEllipse(QPointF(pt.x, pt.y), pt.size, pt.size);
            continue;
        }
        if (pt.life < 0) {
            continue;   // 待机流星：不绘制
        }
        // 流星：亮头 + 沿速度反方向的渐隐拖尾
        const qreal fade = qMin<qreal>(1.0, pt.life / 0.5);     // 收尾渐隐
        QColor head = dark ? QColor(0xF0, 0xF9, 0xFF) : QColor(0x4F, 0x46, 0xE5);
        QColor tail = particleColor(pt.colorIdx);

        const qreal sp = std::sqrt(pt.vx * pt.vx + pt.vy * pt.vy);
        if (sp < 1.0) continue;
        const QPointF dir(-pt.vx / sp, -pt.vy / sp);            // 拖尾指向来路
        const qreal trailLen = sp * 0.32;

        const int headAlpha = static_cast<int>((dark ? 235 : 225) * fade);
        for (int s = 3; s >= 1; --s) {
            const qreal t0 = (s - 1) / 3.0, t1 = s / 3.0;
            QColor seg = tail;
            seg.setAlpha(static_cast<int>(headAlpha * 0.55 * (1.0 - t1)));
            p.setPen(QPen(seg, 1.6, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(pt.x, pt.y) + dir * (trailLen * t0),
                       QPointF(pt.x, pt.y) + dir * (trailLen * t1));
        }
        head.setAlpha(headAlpha);
        p.setPen(Qt::NoPen);
        p.setBrush(head);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size, pt.size);
    }
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

    // ---- 底图（半分辨率缓存放大 blit，模糊感对光晕无损） ----
    p.drawPixmap(rect(), m_baseCache);

    // ---- 粒子层：按当前动效绘制 ----
    switch (m_effect) {
    case Fireflies:     paintFireflies(p);     break;
    case Bubbles:       paintBubbles(p);       break;
    case Snowfall:      paintSnowfall(p);      break;
    case Meteors:       paintMeteors(p);       break;
    case Constellation:
    default:            paintConstellation(p); break;
    }
}
