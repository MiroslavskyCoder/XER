#include "wrapper/qt6/gui/color.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#include <sstream>
#include <cstdio>

#if ENGINE_HAS_QT6
#include <QColor>
#include <QString>
#endif

namespace qt6::gui {

ColorRgba ParseColor(const std::string& css_color) {
#if ENGINE_HAS_QT6
    QColor qc(QString::fromUtf8(css_color.c_str()));
    if (qc.isValid()) return FromQColor(qc);
#endif
    return {};
}

std::string ColorToHex(const ColorRgba& c) {
    char buf[12];
    if (c.a == 255) {
        std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
    } else {
        std::snprintf(buf, sizeof(buf), "#%02x%02x%02x%02x", c.r, c.g, c.b, c.a);
    }
    return buf;
}

std::string ColorToRgbaString(const ColorRgba& c) {
    char buf[48];
    if (c.a == 255) {
        std::snprintf(buf, sizeof(buf), "rgb(%d,%d,%d)", c.r, c.g, c.b);
    } else {
        std::snprintf(buf, sizeof(buf), "rgba(%d,%d,%d,%.3f)", c.r, c.g, c.b, c.a / 255.0);
    }
    return buf;
}

bool IsValidColor(const std::string& css_color) {
#if ENGINE_HAS_QT6
    QColor qc(QString::fromUtf8(css_color.c_str()));
    return qc.isValid();
#else
    return false;
#endif
}

#if ENGINE_HAS_QT6
QColor ParseQColor(const std::string& css_color) {
    return QColor(QString::fromUtf8(css_color.c_str()));
}

ColorRgba FromQColor(const QColor& qc) {
    return { qc.red(), qc.green(), qc.blue(), qc.alpha() };
}

QColor ToQColor(const ColorRgba& c) {
    return QColor(c.r, c.g, c.b, c.a);
}
#endif

}  // namespace qt6::gui
