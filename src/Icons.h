// Icons.h — 程序内绘制图标（QPainter），无需资源文件
#pragma once

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <functional>

namespace Burn {

namespace Icons {

// 在 size x size 画布上绘制一张光盘
inline void drawDisc(QPainter &p, const QRectF &r, const QColor &accent,
                     const QColor &ring, qreal ringWidth = 0.16)
{
    p.setRenderHint(QPainter::Antialiasing);
    const QPointF c = r.center();
    const qreal R = r.width() * 0.5;

    // 银色盘面
    QRadialGradient g(c, R);
    g.setColorAt(0.0, QColor(245, 245, 245));
    g.setColorAt(0.75, QColor(200, 202, 208));
    g.setColorAt(1.0, QColor(160, 162, 168));
    p.setBrush(g);
    p.setPen(QPen(QColor(120, 122, 128), 1));
    p.drawEllipse(r);

    // 彩色数据带
    p.setBrush(ring);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(c.x() - R * (1 - ringWidth), c.y() - R * (1 - ringWidth),
                         R * 2 * (1 - ringWidth), R * 2 * (1 - ringWidth)));

    // 中心高亮环
    p.setBrush(accent);
    p.drawEllipse(QRectF(c.x() - R * 0.34, c.y() - R * 0.34, R * 0.68, R * 0.68));

    // 中心孔
    p.setBrush(QColor(40, 40, 44));
    p.drawEllipse(QRectF(c.x() - R * 0.12, c.y() - R * 0.12, R * 0.24, R * 0.24));
}

// 音乐音符
inline void drawNote(QPainter &p, const QPointF &c, qreal s, const QColor &color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, s * 0.10, Qt::SolidLine, Qt::RoundCap));
    // 符干
    p.drawLine(QPointF(c.x() + s * 0.22, c.y() - s * 0.10),
               QPointF(c.x() + s * 0.22, c.y() - s * 0.85));
    // 符头
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(c.x() - s * 0.10, c.y() - s * 0.32, s * 0.36, s * 0.32));
    // 符尾
    QPainterPath tail;
    tail.moveTo(c.x() + s * 0.22, c.y() - s * 0.85);
    tail.cubicTo(c.x() + s * 0.55, c.y() - s * 0.70,
                 c.x() + s * 0.40, c.y() - s * 0.42,
                 c.x() + s * 0.22, c.y() - s * 0.10);
    p.drawPath(tail);
    p.restore();
}

// 向下箭头
inline void drawArrowDown(QPainter &p, const QPointF &c, qreal s, const QColor &color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, s * 0.12, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(QPointF(c.x(), c.y() - s * 0.45), QPointF(c.x(), c.y() + s * 0.30));
    QPainterPath head;
    head.moveTo(c.x() - s * 0.24, c.y() + s * 0.06);
    head.lineTo(c.x(), c.y() + s * 0.30);
    head.lineTo(c.x() + s * 0.24, c.y() + s * 0.06);
    p.setBrush(color);
    p.drawPath(head);
    p.restore();
}

// 放大镜
inline void drawMagnifier(QPainter &p, const QPointF &c, qreal s, const QColor &color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, s * 0.14, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(color);
    p.drawEllipse(QRectF(c.x() - s * 0.34, c.y() - s * 0.38, s * 0.48, s * 0.48));
    p.setBrush(Qt::transparent);
    p.drawEllipse(QRectF(c.x() - s * 0.26, c.y() - s * 0.30, s * 0.32, s * 0.32));
    p.drawLine(QPointF(c.x() + s * 0.05, c.y() + s * 0.05),
               QPointF(c.x() + s * 0.35, c.y() + s * 0.35));
    p.restore();
}

inline QPixmap pix(int size, const std::function<void(QPainter &, int)> &draw)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    draw(p, size);
    p.end();
    return pm;
}

// 数据光盘
inline QIcon dataDisc()
{
    return QIcon(pix(64, [](QPainter &p, int s) {
        drawDisc(p, QRectF(2, 2, s - 4, s - 4), QColor(41, 121, 255),
                 QColor(140, 180, 255), 0.24);
    }));
}

// 音乐 CD
inline QIcon musicDisc()
{
    return QIcon(pix(64, [](QPainter &p, int s) {
        drawDisc(p, QRectF(2, 2, s - 4, s - 4), QColor(232, 62, 140),
                 QColor(255, 180, 210), 0.24);
        drawNote(p, QPointF(s * 0.5, s * 0.5), s * 0.42, QColor(255, 255, 255));
    }));
}

// 光盘复制（两张盘）
inline QIcon copyDisc()
{
    return QIcon(pix(64, [](QPainter &p, int s) {
        drawDisc(p, QRectF(4, 8, s - 22, s - 22), QColor(52, 168, 83),
                 QColor(170, 230, 190), 0.2);
        p.save();
        p.setOpacity(0.55);
        drawDisc(p, QRectF(16, 2, s - 24, s - 24), QColor(46, 125, 50),
                 QColor(140, 200, 150), 0.2);
        p.restore();
    }));
}

// ISO 镜像
inline QIcon isoImage()
{
    return QIcon(pix(64, [](QPainter &p, int s) {
        drawDisc(p, QRectF(2, 2, s - 4, s - 4), QColor(255, 143, 0),
                 QColor(255, 214, 150), 0.24);
        drawArrowDown(p, QPointF(s * 0.5, s * 0.46), s * 0.34, QColor(255, 255, 255));
    }));
}

// 介质信息
inline QIcon mediaInfo()
{
    return QIcon(pix(64, [](QPainter &p, int s) {
        drawDisc(p, QRectF(6, 10, s - 30, s - 30), QColor(120, 86, 255),
                 QColor(190, 170, 255), 0.22);
        drawMagnifier(p, QPointF(s * 0.66, s * 0.42), s * 0.34, QColor(120, 86, 255));
    }));
}

// 擦除
inline QIcon erase()
{
    return QIcon(pix(64, [](QPainter &p, int s) {
        drawDisc(p, QRectF(2, 2, s - 4, s - 4), QColor(96, 98, 104),
                 QColor(180, 182, 190), 0.24);
        // 对角线 ×
        p.setPen(QPen(QColor(229, 57, 53), 5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(s * 0.30, s * 0.30), QPointF(s * 0.70, s * 0.70));
        p.drawLine(QPointF(s * 0.70, s * 0.30), QPointF(s * 0.30, s * 0.70));
    }));
}

// 弹出（托盘 + 上箭头）
inline QIcon eject()
{
    return QIcon(pix(48, [](QPainter &p, int s) {
        p.setRenderHint(QPainter::Antialiasing);
        const QColor c(70, 74, 82);
        // 托盘
        p.setPen(QPen(c, 2.4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(s * 0.16, s * 0.78), QPointF(s * 0.84, s * 0.78));
        // 箭头
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        QPainterPath head;
        head.moveTo(s * 0.5, s * 0.16);
        head.lineTo(s * 0.30, s * 0.40);
        head.lineTo(s * 0.70, s * 0.40);
        head.closeSubpath();
        p.drawPath(head);
        // 箭头杆
        p.setPen(QPen(c, 2.4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(s * 0.5, s * 0.30), QPointF(s * 0.5, s * 0.66));
    }));
}

// 播放 / 烧录（三角）
inline QIcon burn()
{
    return QIcon(pix(48, [](QPainter &p, int s) {
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath tri;
        tri.moveTo(s * 0.32, s * 0.22);
        tri.lineTo(s * 0.78, s * 0.50);
        tri.lineTo(s * 0.32, s * 0.78);
        tri.closeSubpath();
        p.setBrush(QColor(41, 121, 255));
        p.setPen(Qt::NoPen);
        p.drawPath(tri);
    }));
}

// 三角（主操作按钮用）
inline QIcon play(const QColor &color)
{
    return QIcon(pix(24, [color](QPainter &p, int s) {
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath tri;
        tri.moveTo(s * 0.28, s * 0.18);
        tri.lineTo(s * 0.80, s * 0.50);
        tri.lineTo(s * 0.28, s * 0.82);
        tri.closeSubpath();
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawPath(tri);
    }));
}

} // namespace Icons
} // namespace Burn
