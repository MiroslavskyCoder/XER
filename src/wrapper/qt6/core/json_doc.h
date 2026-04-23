#pragma once
#include <string>
#include <v8.h>
#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::core {

// ---------------------------------------------------------------------------
// QtJson — exposed as global object (not a class), like JSON but Qt-backed.
// Supports compact/indented modes and JSON5-ish error reporting.
//
// JS API:
//   QtJson.parse(text: string) → any
//   QtJson.stringify(value: any, indent?: number) → string
//   QtJson.parseFile(path: string) → any
//   QtJson.writeFile(value: any, path: string, indent?: number) → bool
//   QtJson.isValid(text: string) → bool
// ---------------------------------------------------------------------------
bool RegisterQtJsonModule(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::core
