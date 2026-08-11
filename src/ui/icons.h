#ifndef ICONS_H
#define ICONS_H

#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>

// 轻量自绘矢量图标集：线性风格、圆角端点，风格统一，零外部依赖。
// 用法：QPixmap pm = Icons::pixmap(Icons::Plus, 16, QColor("#3b82f6"));
namespace Icons {

enum Type {
    Plus,
    Folder,
    Calendar,
    Tag,
    Pin,
    Trash,
    Check,
    ChevronLeft,
    ChevronRight,
    ChevronsLeft,   // <<
    ChevronsRight,  // >>
    Desktop,
    Close,
    Save,
    List,
    Edit,
    Clock,
    Import,
    Export,
    Backup,
    Exit
};

namespace detail {

inline void addStrokePath(QPainterPath &path, Type t)
{
    switch (t) {
    case Plus:
        path.moveTo(8, 3.5);  path.lineTo(8, 12.5);
        path.moveTo(3.5, 8);  path.lineTo(12.5, 8);
        break;
    case Folder:
        path.moveTo(2.5, 4.5);
        path.lineTo(6.0, 4.5);
        path.lineTo(7.5, 6.0);
        path.lineTo(13.5, 6.0);
        path.addRoundedRect(2.5, 4.5, 11, 8.5, 1.2, 1.2);
        break;
    case Calendar:
        path.addRoundedRect(2.8, 4.0, 10.4, 9.0, 1.4, 1.4);
        path.moveTo(2.8, 7.2); path.lineTo(13.2, 7.2);
        path.moveTo(5.6, 2.6); path.lineTo(5.6, 5.2);
        path.moveTo(10.4, 2.6); path.lineTo(10.4, 5.2);
        break;
    case Tag:
        path.moveTo(3, 3);
        path.lineTo(8.6, 3);
        path.lineTo(13.2, 7.6);
        path.quadTo(13.9, 8.3, 13.2, 9.0);
        path.lineTo(9.0, 13.2);
        path.quadTo(8.3, 13.9, 7.6, 13.2);
        path.lineTo(3, 8.6);
        path.closeSubpath();
        path.moveTo(6.1, 6.1); path.lineTo(6.2, 6.1); // 圆点由圆帽线模拟
        break;
    case Pin:
        path.moveTo(9.5, 2.8); path.lineTo(13.2, 6.5);
        path.moveTo(9.2, 3.2);
        path.lineTo(6.4, 3.4);
        path.lineTo(3.2, 9.2);
        path.lineTo(6.8, 12.8);
        path.lineTo(12.6, 9.6);
        path.lineTo(12.8, 6.8);
        path.moveTo(6.0, 10.0); path.lineTo(2.8, 13.2);
        break;
    case Trash:
        path.moveTo(3.0, 4.5);  path.lineTo(13.0, 4.5);
        path.moveTo(6.0, 4.5);  path.lineTo(6.0, 3.2);  path.lineTo(10.0, 3.2); path.lineTo(10.0, 4.5);
        path.moveTo(4.2, 4.5);  path.lineTo(5.0, 13.2); path.lineTo(11.0, 13.2); path.lineTo(11.8, 4.5);
        path.moveTo(7.0, 7.0);  path.lineTo(7.4, 11.0);
        path.moveTo(9.0, 7.0);  path.lineTo(8.6, 11.0);
        break;
    case Check:
        path.moveTo(3.2, 8.4); path.lineTo(6.6, 11.8); path.lineTo(12.8, 4.4);
        break;
    case ChevronLeft:
        path.moveTo(9.8, 3.6); path.lineTo(5.4, 8.0); path.lineTo(9.8, 12.4);
        break;
    case ChevronRight:
        path.moveTo(6.2, 3.6); path.lineTo(10.6, 8.0); path.lineTo(6.2, 12.4);
        break;
    case ChevronsLeft:
        path.moveTo(8.6, 3.6); path.lineTo(4.2, 8.0); path.lineTo(8.6, 12.4);
        path.moveTo(12.6, 3.6); path.lineTo(8.2, 8.0); path.lineTo(12.6, 12.4);
        break;
    case ChevronsRight:
        path.moveTo(3.4, 3.6); path.lineTo(7.8, 8.0); path.lineTo(3.4, 12.4);
        path.moveTo(7.4, 3.6); path.lineTo(11.8, 8.0); path.lineTo(7.4, 12.4);
        break;
    case Desktop:
        path.addRoundedRect(2.4, 3.4, 11.2, 7.4, 1.0, 1.0);
        path.moveTo(6.2, 12.8); path.lineTo(9.8, 12.8);
        path.moveTo(8.0, 10.8); path.lineTo(8.0, 12.8);
        break;
    case Close:
        path.moveTo(4.2, 4.2); path.lineTo(11.8, 11.8);
        path.moveTo(11.8, 4.2); path.lineTo(4.2, 11.8);
        break;
    case Save:
        path.moveTo(3.4, 3.2); path.lineTo(11.0, 3.2); path.lineTo(12.8, 5.0); path.lineTo(12.8, 12.8); path.lineTo(3.2, 12.8); path.closeSubpath();
        path.moveTo(5.4, 3.2); path.lineTo(5.4, 6.4); path.lineTo(10.4, 6.4); path.lineTo(10.4, 3.2);
        path.addRoundedRect(5.6, 9.0, 4.8, 3.8, 0.6, 0.6);
        break;
    case List:
        path.moveTo(5.6, 4.2); path.lineTo(13.0, 4.2);
        path.moveTo(5.6, 8.0); path.lineTo(13.0, 8.0);
        path.moveTo(5.6, 11.8); path.lineTo(13.0, 11.8);
        path.moveTo(3.0, 4.2); path.lineTo(3.1, 4.2);
        path.moveTo(3.0, 8.0); path.lineTo(3.1, 8.0);
        path.moveTo(3.0, 11.8); path.lineTo(3.1, 11.8);
        break;
    case Edit:
        path.moveTo(9.4, 3.6); path.lineTo(12.4, 6.6);
        path.moveTo(3.0, 13.0); path.lineTo(3.6, 10.0); path.lineTo(10.2, 3.4); path.lineTo(12.6, 5.8); path.lineTo(6.0, 12.4); path.closeSubpath();
        break;
    case Clock:
        path.addEllipse(3.0, 3.0, 10.0, 10.0);
        path.moveTo(8.0, 5.2); path.lineTo(8.0, 8.0); path.lineTo(10.6, 9.8);
        break;
    case Import:
        path.moveTo(8.0, 2.8); path.lineTo(8.0, 10.4);
        path.moveTo(4.8, 7.2); path.lineTo(8.0, 10.4); path.lineTo(11.2, 7.2);
        path.moveTo(3.0, 12.8); path.lineTo(13.0, 12.8);
        break;
    case Export:
        path.moveTo(8.0, 10.4); path.lineTo(8.0, 2.8);
        path.moveTo(4.8, 6.0); path.lineTo(8.0, 2.8); path.lineTo(11.2, 6.0);
        path.moveTo(3.0, 12.8); path.lineTo(13.0, 12.8);
        break;
    case Backup:
        path.moveTo(13.0, 8.0);
        path.arcTo(3.0, 3.0, 10.0, 10.0, 0, 300);
        path.moveTo(13.0, 3.2); path.lineTo(13.0, 5.6); path.lineTo(10.6, 5.6);
        path.moveTo(8.0, 5.4); path.lineTo(8.0, 8.0); path.lineTo(10.2, 9.4);
        break;
    case Exit:
        path.moveTo(6.2, 3.2); path.lineTo(3.2, 3.2); path.lineTo(3.2, 12.8); path.lineTo(6.2, 12.8);
        path.moveTo(9.8, 5.4); path.lineTo(12.8, 8.0); path.lineTo(9.8, 10.6);
        path.moveTo(12.8, 8.0); path.lineTo(6.0, 8.0);
        break;
    }
}

} // namespace detail

// 在 16x16 逻辑坐标系中描边绘制，缩放到目标尺寸
inline QPixmap pixmap(Type type, int size, const QColor &color, qreal stroke = 1.7)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(size / 16.0, size / 16.0);

    QPainterPath path;
    detail::addStrokePath(path, type);

    QPen pen(color, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
    painter.end();

    return pm;
}

inline QIcon icon(Type type, int size, const QColor &color)
{
    return QIcon(pixmap(type, size, color));
}

} // namespace Icons

#endif // ICONS_H
