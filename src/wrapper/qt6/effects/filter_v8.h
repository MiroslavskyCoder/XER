#pragma once
#include <v8.h>

namespace qt6::effects {

// Registers global module object QtImageFilter.
// JS API:
//   QtImageFilter.blur(src, dst, radius?, gaussian?) -> bool
//   QtImageFilter.dropShadow(src, dst, blurRadius?, offsetX?, offsetY?, color?) -> bool
//   QtImageFilter.glow(src, dst, radius?, color?) -> bool
//   QtImageFilter.adjustBrightnessContrast(src, dst, brightness?, contrast?) -> bool
//   QtImageFilter.tint(src, dst, color) -> bool
bool RegisterQtImageFilterModule(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::effects
