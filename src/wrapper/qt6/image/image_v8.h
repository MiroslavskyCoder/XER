#pragma once
#include <v8.h>

namespace qt6::image {

// Registers global module object QtImage.
// JS API:
//   QtImage.info(path) -> { width, height, depth, format, valid }
//   QtImage.resize(src, dst, width, height, keepAspect?) -> bool
//   QtImage.convert(src, dst, quality?) -> bool
//   QtImage.crop(src, dst, x, y, width, height) -> bool
//   QtImage.flipH(src, dst) -> bool
//   QtImage.flipV(src, dst) -> bool
//   QtImage.rotate(src, dst, degrees) -> bool
//   QtImage.toGrayscale(src, dst) -> bool
bool RegisterQtImageModule(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::image
