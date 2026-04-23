#pragma once

#include <string>
#include <vector>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QPointF>
#endif

namespace qt6::graphics {

struct Point { double x = 0, y = 0; };
struct Rect  { double x = 0, y = 0, w = 0, h = 0; };

// SVG-like path builder
struct PathBuilder {
    std::vector<std::string> commands;

    PathBuilder& MoveTo(double x, double y);
    PathBuilder& LineTo(double x, double y);
    PathBuilder& CubicTo(double cx1, double cy1,
                          double cx2, double cy2,
                          double ex,  double ey);
    PathBuilder& QuadTo(double cx, double cy, double ex, double ey);
    PathBuilder& ArcTo(double rx, double ry, double angle,
                        bool large, bool sweep, double ex, double ey);
    PathBuilder& Close();

    // Render to PNG using QPainter (requires Qt6::Gui)
    bool RenderToPng(const std::string& out_path,
                     int width, int height,
                     const std::string& stroke_color = "#000000",
                     const std::string& fill_color   = "transparent",
                     double stroke_width = 1.0) const;

#if ENGINE_HAS_QT6
    QPainterPath ToQPainterPath() const;
#endif
};

// Simple drawing helpers
bool DrawRect(const std::string& out_png, int width, int height,
              const Rect& rect,
              const std::string& fill  = "#ffffff",
              const std::string& stroke = "#000000",
              double stroke_width = 1.0);

bool DrawText(const std::string& out_png, int width, int height,
              const std::string& text,
              double x, double y,
              const std::string& color     = "#000000",
              const std::string& font_name = "Arial",
              int font_size = 12);

}  // namespace qt6::graphics
