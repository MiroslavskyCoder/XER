#pragma once

#include <string>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QColor>
#endif

namespace qt6::gui {

struct ColorRgba {
    int r = 0, g = 0, b = 0, a = 255;
};

ColorRgba ParseColor(const std::string& css_color);
std::string ColorToHex(const ColorRgba& c);
std::string ColorToRgbaString(const ColorRgba& c);
bool IsValidColor(const std::string& css_color);

#if ENGINE_HAS_QT6
QColor ParseQColor(const std::string& css_color);
ColorRgba FromQColor(const QColor& qc);
QColor ToQColor(const ColorRgba& c);
#endif

}  // namespace qt6::gui
