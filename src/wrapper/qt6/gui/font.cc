#include "wrapper/qt6/gui/font.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#include <sstream>

#if ENGINE_HAS_QT6
#include <QFont>
#include <QString>
#endif

namespace qt6::gui {

FontDesc ParseFont(const std::string& family, int point_size, bool bold, bool italic) {
    FontDesc f;
    f.family     = family;
    f.point_size = point_size;
    f.bold       = bold;
    f.italic     = italic;
    f.weight     = bold ? 700 : 400;
    return f;
}

std::string FontToString(const FontDesc& f) {
    std::ostringstream ss;
    if (f.italic) ss << "italic ";
    if (f.bold)   ss << "bold ";
    ss << f.point_size << "pt " << f.family;
    return ss.str();
}

#if ENGINE_HAS_QT6
QFont ToQFont(const FontDesc& f) {
    QFont qf(QString::fromUtf8(f.family.c_str()), f.point_size,
              f.bold ? QFont::Bold : QFont::Normal, f.italic);
    return qf;
}

FontDesc FromQFont(const QFont& qf) {
    FontDesc f;
    f.family     = qf.family().toUtf8().constData();
    f.point_size = qf.pointSize();
    f.bold       = qf.bold();
    f.italic     = qf.italic();
    f.weight     = qf.weight();
    return f;
}
#endif

}  // namespace qt6::gui
