#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QFont>
#include <QString>
#include <QStringList>

// 全局设计令牌：颜色 / 字体 / 尺寸统一在此定义，保证所有页面风格一致
// 支持浅色 / 深色双主题，Theme::setDark() 切换后所有取色函数自动切换
namespace Theme {

// ---- 主题模式 ----
inline bool &darkFlagRef() { static bool dark = false; return dark; }
inline bool isDark()       { return darkFlagRef(); }
inline void setDark(bool dark) { darkFlagRef() = dark; }

// ---- 主色系（清新蓝） ----
inline QColor primary()        { return isDark() ? QColor(0x60, 0x8C, 0xFA) : QColor(0x3B, 0x82, 0xF6); }
inline QColor primaryHover()   { return isDark() ? QColor(0x3B, 0x82, 0xF6) : QColor(0x25, 0x63, 0xEB); }
inline QColor primarySoft()    { return isDark() ? QColor(0x1E, 0x2A, 0x44) : QColor(0xEF, 0xF6, 0xFF); }
inline QColor primarySoft2()   { return isDark() ? QColor(0x25, 0x35, 0x55) : QColor(0xDB, 0xEA, 0xFE); }

// ---- 功能色 ----
inline QColor success()        { return isDark() ? QColor(0x34, 0xD3, 0x99) : QColor(0x10, 0xB9, 0x81); }
inline QColor warning()        { return isDark() ? QColor(0xFB, 0xB4, 0x24) : QColor(0xF5, 0x9E, 0x0B); }
inline QColor danger()         { return isDark() ? QColor(0xF8, 0x71, 0x71) : QColor(0xEF, 0x44, 0x44); }
inline QColor accent()         { return isDark() ? QColor(0xA7, 0x8B, 0xFA) : QColor(0x8B, 0x5C, 0xF6); }

// ---- 中性色 ----
inline QColor background()     { return isDark() ? QColor(0x0F, 0x17, 0x26) : QColor(0xF6, 0xF8, 0xFB); }
inline QColor surface()        { return isDark() ? QColor(0x1A, 0x23, 0x35) : QColor(0xFF, 0xFF, 0xFF); }
inline QColor surfaceAlt()     { return isDark() ? QColor(0x21, 0x2C, 0x42) : QColor(0xF1, 0xF5, 0xF9); }
inline QColor border()         { return isDark() ? QColor(0x2E, 0x3B, 0x52) : QColor(0xE2, 0xE8, 0xF0); }
inline QColor borderStrong()   { return isDark() ? QColor(0x3F, 0x4E, 0x69) : QColor(0xCB, 0xD5, 0xE1); }
inline QColor textPrimary()    { return isDark() ? QColor(0xE6, 0xEC, 0xF5) : QColor(0x1E, 0x29, 0x3B); }
inline QColor textSecondary()  { return isDark() ? QColor(0x9A, 0xA8, 0xBC) : QColor(0x64, 0x74, 0x8B); }
inline QColor textMuted()      { return isDark() ? QColor(0x6B, 0x7A, 0x90) : QColor(0x94, 0xA3, 0xB8); }
inline QColor textDisabled()   { return isDark() ? QColor(0x4A, 0x56, 0x68) : QColor(0xB4, 0xB9, 0xBE); }

// ---- 标签/文件夹调色板 ----
inline QStringList palette()
{
    return { QStringLiteral("#3b82f6"), QStringLiteral("#10b981"),
             QStringLiteral("#ef4444"), QStringLiteral("#f59e0b"),
             QStringLiteral("#8b5cf6"), QStringLiteral("#06b6d4") };
}

// ---- 尺寸 ----
constexpr int radiusSm = 6;
constexpr int radiusMd = 10;
constexpr int radiusLg = 14;
constexpr int spacing  = 12;

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

} // namespace Theme

#endif // THEME_H
