#include "wrapper/qt6/graphics/painter.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#include <sstream>

#if ENGINE_HAS_QT6
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QString>
#endif

namespace qt6::graphics {

PathBuilder& PathBuilder::MoveTo(double x, double y) {
    std::ostringstream s;
    s << "M " << x << " " << y;
    commands.push_back(s.str());
    return *this;
}

PathBuilder& PathBuilder::LineTo(double x, double y) {
    std::ostringstream s;
    s << "L " << x << " " << y;
    commands.push_back(s.str());
    return *this;
}

PathBuilder& PathBuilder::CubicTo(double cx1, double cy1,
                                  double cx2, double cy2,
                                  double ex, double ey) {
    std::ostringstream s;
    s << "C " << cx1 << " " << cy1 << " "
              << cx2 << " " << cy2 << " "
              << ex  << " " << ey;
    commands.push_back(s.str());
    return *this;
}

PathBuilder& PathBuilder::QuadTo(double cx, double cy, double ex, double ey) {
    std::ostringstream s;
    s << "Q " << cx << " " << cy << " " << ex << " " << ey;
    commands.push_back(s.str());
    return *this;
}

PathBuilder& PathBuilder::ArcTo(double rx, double ry, double angle,
                                bool large, bool sweep,
                                double ex, double ey) {
    std::ostringstream s;
    s << "A " << rx << " " << ry << " " << angle
              << " " << (large ? 1 : 0) << " " << (sweep ? 1 : 0)
              << " " << ex << " " << ey;
    commands.push_back(s.str());
    return *this;
}

PathBuilder& PathBuilder::Close() {
    commands.push_back("Z");
    return *this;
}

#if ENGINE_HAS_QT6
QPainterPath PathBuilder::ToQPainterPath() const {
    QPainterPath path;
    for (const auto& cmd : commands) {
        char type = cmd[0];
        std::istringstream is(cmd.substr(2));
        if (type == 'M') {
            double x, y;
            is >> x >> y;
            path.moveTo(x, y);
        } else if (type == 'L') {
            double x, y;
            is >> x >> y;
            path.lineTo(x, y);
        } else if (type == 'C') {
            double cx1, cy1, cx2, cy2, ex, ey;
            is >> cx1 >> cy1 >> cx2 >> cy2 >> ex >> ey;
            path.cubicTo(cx1, cy1, cx2, cy2, ex, ey);
        } else if (type == 'Q') {
            double cx, cy, ex, ey;
            is >> cx >> cy >> ex >> ey;
            path.quadTo(cx, cy, ex, ey);
        } else if (type == 'Z') {
            path.closeSubpath();
        }
    }
    return path;
}
#endif

bool PathBuilder::RenderToPng(const std::string& out_path,
                              int width, int height,
                              const std::string& stroke_color,
                              const std::string& fill_color,
                              double stroke_width) const {
#if ENGINE_HAS_QT6
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath qpath = ToQPainterPath();

    if (fill_color != "transparent") {
        painter.fillPath(qpath, QColor(QString::fromUtf8(fill_color.c_str())));
    }

    QPen pen(QColor(QString::fromUtf8(stroke_color.c_str())));
    pen.setWidthF(stroke_width);
    painter.setPen(pen);
    painter.drawPath(qpath);
    painter.end();

    return img.save(QString::fromUtf8(out_path.c_str()), "PNG");
#else
    return false;
#endif
}

}  // namespace qt6::graphics
