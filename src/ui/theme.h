#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QFont>
#include <QString>
#include <QStringList>

// 全局设计令牌：颜色 / 字体 / 尺寸统一在此定义，保证所有页面风格一致
// 支持浅色 / 深色双主题，Theme::setDark() 切换后所有取色函数自动切换
// 深色主题为「暗夜霓虹」：深空底色 + 青紫霓虹 + 玻璃拟态
namespace Theme {

// ---- 主题模式 ----
inline bool &darkFlagRef() { static bool dark = true; return dark; }
inline bool isDark()       { return darkFlagRef(); }
inline void setDark(bool dark) { darkFlagRef() = dark; }

// ---- 主色系（霓虹青紫） ----
inline QColor primary()        { return isDark() ? QColor(0x22, 0xD3, 0xEE) : QColor(0x4F, 0x46, 0xE5); }
inline QColor primaryHover()   { return isDark() ? QColor(0x67, 0xE8, 0xF9) : QColor(0x43, 0x38, 0xCA); }
inline QColor primarySoft()    { return isDark() ? QColor(0x16, 0x3A, 0x4A) : QColor(0xE0, 0xE7, 0xFF); }
inline QColor primarySoft2()   { return isDark() ? QColor(0x1D, 0x4A, 0x5E) : QColor(0xC7, 0xD2, 0xFE); }

// ---- 功能色 ----
inline QColor success()        { return isDark() ? QColor(0x34, 0xD3, 0x99) : QColor(0x10, 0xB9, 0x81); }
inline QColor warning()        { return isDark() ? QColor(0xFB, 0xB4, 0x24) : QColor(0xF5, 0x9E, 0x0B); }
inline QColor danger()         { return isDark() ? QColor(0xF8, 0x71, 0x71) : QColor(0xEF, 0x44, 0x44); }
inline QColor accent()         { return isDark() ? QColor(0xA8, 0x55, 0xF7) : QColor(0x8B, 0x5C, 0xF6); }
inline QColor neonPink()       { return isDark() ? QColor(0xF4, 0x72, 0xB6) : QColor(0xEC, 0x48, 0x99); }

// ---- 中性色 ----
inline QColor background()     { return isDark() ? QColor(0x0B, 0x0E, 0x1A) : QColor(0xE8, 0xED, 0xF6); }
inline QColor surface()        { return isDark() ? QColor(0x14, 0x18, 0x2A) : QColor(0xFF, 0xFF, 0xFF); }
inline QColor surfaceAlt()     { return isDark() ? QColor(0x1B, 0x21, 0x38) : QColor(0xE3, 0xE9, 0xF3); }
inline QColor border()         { return isDark() ? QColor(0x2A, 0x33, 0x4E) : QColor(0xD4, 0xDC, 0xEA); }
inline QColor borderStrong()   { return isDark() ? QColor(0x3D, 0x4A, 0x6B) : QColor(0xB8, 0xC4, 0xD8); }
inline QColor textPrimary()    { return isDark() ? QColor(0xEA, 0xF0, 0xFA) : QColor(0x1E, 0x29, 0x3B); }
inline QColor textSecondary()  { return isDark() ? QColor(0x9D, 0xAC, 0xC6) : QColor(0x55, 0x65, 0x80); }
inline QColor textMuted()      { return isDark() ? QColor(0x6E, 0x7E, 0x9C) : QColor(0x85, 0x93, 0xAB); }
inline QColor textDisabled()   { return isDark() ? QColor(0x51, 0x5E, 0x76) : QColor(0xAD, 0xB6, 0xC4); }

// ---- 尺寸 ----
constexpr int radiusSm = 6;
constexpr int radiusMd = 10;
constexpr int radiusLg = 14;
constexpr int spacing  = 12;

// ---- 玻璃拟态（深色：半透明白 + 高光描边；浅色：半透明白卡片） ----
inline QColor glassBg()        { return isDark() ? QColor(255, 255, 255, 10)  : QColor(255, 255, 255, 200); }
inline QColor glassBgStrong()  { return isDark() ? QColor(255, 255, 255, 18)  : QColor(255, 255, 255, 225); }
inline QColor glassBorder()    { return isDark() ? QColor(255, 255, 255, 26)  : QColor(183, 194, 214, 200); }
inline QColor glassHighlight() { return isDark() ? QColor(255, 255, 255, 42)  : QColor(255, 255, 255, 255); }
inline QColor hoverGlow()      { return isDark() ? QColor(0x22, 0xD3, 0xEE, 26) : QColor(0x4F, 0x46, 0xE5, 18); }

// 玻璃面板的内联样式（供 C++ 中 setStyleSheet 烘焙使用）
inline QString glassPanelStyle(const QString &objectName)
{
    return QStringLiteral(
               "QWidget#%1 { background-color: %2; border: 1px solid %3; border-radius: %4px; }")
        .arg(objectName,
             glassBg().name(QColor::HexArgb),
             glassBorder().name(QColor::HexArgb))
        .arg(radiusLg);
}

// ---- 标签/文件夹调色板 ----
inline QStringList palette()
{
    return { QStringLiteral("#22d3ee"), QStringLiteral("#34d399"),
             QStringLiteral("#f87171"), QStringLiteral("#fbbf24"),
             QStringLiteral("#a855f7"), QStringLiteral("#f472b6") };
}

// ---- 字号（加大，提升可读性） ----
constexpr int fontSmall  = 12;
constexpr int fontBase   = 14;
constexpr int fontMedium = 15;
constexpr int fontTitle  = 17;
constexpr int fontHero   = 20;

inline QFont font(int pixelSize = fontBase, QFont::Weight weight = QFont::Normal)
{
    QFont f(QStringLiteral("Microsoft YaHei UI"));
    f.setPixelSize(pixelSize);
    f.setWeight(weight);
    return f;
}

// 辅助：颜色加透明度
inline QColor withAlpha(QColor c, int alpha)
{
    c.setAlpha(alpha);
    return c;
}

// 辅助：在两色之间插值（0~1）
inline QColor mix(const QColor &a, const QColor &b, qreal t)
{
    return QColor(
        a.red()   + static_cast<int>((b.red()   - a.red())   * t),
        a.green() + static_cast<int>((b.green() - a.green()) * t),
        a.blue()  + static_cast<int>((b.blue()  - a.blue())  * t),
        a.alpha() + static_cast<int>((b.alpha() - a.alpha()) * t));
}

} // namespace Theme

#endif // THEME_H
