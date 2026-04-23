# Qt6 Integration

This directory is reserved for core Qt6 integration code used by the engine.
 
Suggested usage:
- Qt runtime/bootstrap helpers
- Qt/V8 bridge helpers
- shared Qt6 utility wrappers reused outside script modules

Directory layout:
- `v8/` - V8 bridge helpers for exposing Qt6 objects and values into scripts
- `util/` - shared utility helpers used by other Qt6 wrappers
- `core/` - wrappers for Qt6 Core functionality such as paths, strings, timers, json
- `gui/` - wrappers for Qt6 GUI types such as colors, fonts, windows, clipboard
- `ui/` - widget-oriented and UI composition helpers
- `web/` - wrappers for Qt6 web stack and browser-oriented helpers
- `xml/` - wrappers for Qt6 XML reader/writer and DOM helpers
- `pdf/` - wrappers for Qt6 PDF rendering and document helpers
- `graphics/` - wrappers for scenes, painters, transforms and related graphics helpers
- `effects/` - wrappers for Qt6 visual effects and style-related helpers
- `image/` - wrappers for QImage, pixmaps, codecs and image processing helpers