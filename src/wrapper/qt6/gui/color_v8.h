#pragma once
#include <v8.h>

namespace qt6::gui {

// Exports global module object QtColor with helper methods.
// JS API:
//   QtColor.parse(css: string) -> { r, g, b, a }
//   QtColor.toHex(r, g, b, a?) -> string
//   QtColor.toRgba(r, g, b, a?) -> string
//   QtColor.isValid(css: string) -> bool
bool RegisterQtColorModule(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::gui
