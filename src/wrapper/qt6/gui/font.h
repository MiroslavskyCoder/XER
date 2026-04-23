#pragma once

#include <string>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QFont>
#endif

namespace qt6::gui {

struct FontDesc {
    std::string family;
    int         point_size  = 12;
    bool        bold        = false;
    bool        italic      = false;
    int         weight      = 400;  // CSS-style weight (100-900)
};

FontDesc ParseFont(const std::string& family, int point_size = 12,
                   bool bold = false, bool italic = false);
std::string FontToString(const FontDesc& f);

#if ENGINE_HAS_QT6
QFont ToQFont(const FontDesc& f);
FontDesc FromQFont(const QFont& qf);
#endif

}  // namespace qt6::gui
