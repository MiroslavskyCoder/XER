#include "wrapper/qt6/graphics/painter.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QColor>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QString>
#endif

namespace qt6::graphics {

bool DrawRect(const std::string& out_png, int width, int height,
              const Rect& rect,
              const std::string& fill,
              const std::string& stroke,
              double stroke_width) {
#if ENGINE_HAS_QT6
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF qrect(rect.x, rect.y, rect.w, rect.h);
    if (fill != "transparent") {
        painter.fillRect(qrect, QColor(QString::fromUtf8(fill.c_str())));
    }
    QPen pen(QColor(QString::fromUtf8(stroke.c_str())));
    pen.setWidthF(stroke_width);
    painter.setPen(pen);
    painter.drawRect(qrect);
    painter.end();

    return img.save(QString::fromUtf8(out_png.c_str()), "PNG");
#else
    return false;
#endif
}

bool DrawText(const std::string& out_png, int width, int height,
              const std::string& text,
              double x, double y,
              const std::string& color,
              const std::string& font_name,
              int font_size) {
#if ENGINE_HAS_QT6
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QFont font(QString::fromUtf8(font_name.c_str()), font_size);
    painter.setFont(font);
    painter.setPen(QColor(QString::fromUtf8(color.c_str())));
    painter.drawText(QPointF(x, y), QString::fromUtf8(text.c_str()));
    painter.end();

    return img.save(QString::fromUtf8(out_png.c_str()), "PNG");
#else
    return false;
#endif
}

}  // namespace qt6::graphics
