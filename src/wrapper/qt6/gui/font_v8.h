#pragma once
#include <v8.h>

namespace qt6::gui {

// Exports global module object QtFont with helper methods.
// JS API:
//   QtFont.parse(family, size?, bold?, italic?) -> { family, pointSize, bold, italic, weight }
//   QtFont.toString(fontObj) -> string
bool RegisterQtFontModule(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::gui
