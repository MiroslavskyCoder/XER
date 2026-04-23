#pragma once
#include <v8.h>

namespace qt6::graphics {

// Registers global class QtPainterPath.
// JS API (instance):
//   new QtPainterPath()
//   moveTo(x, y), lineTo(x, y), close()
//   cubicTo(cx1,cy1,cx2,cy2,ex,ey), quadTo(cx,cy,ex,ey)
//   arcTo(rx,ry,angle,large,sweep,ex,ey)
//   renderToPng(outPath, width, height, stroke?, fill?, strokeWidth?) -> bool
// Registers global module QtDraw:
//   QtDraw.rect(outPng, width, height, {x,y,w,h}, fill?, stroke?, strokeWidth?) -> bool
//   QtDraw.text(outPng, width, height, text, x, y, color?, fontName?, fontSize?) -> bool
bool RegisterQtPainterClass(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::graphics
