# Qt6 V8 Modules

This directory contains Qt6-backed modules exposed to the V8 runtime.

Clean structure:
- each module is implemented by exactly two files:
- `<module>_bind.cc` (exports global object and methods)
- `<module>_callbacks.cc` (runtime callbacks and argument checks)
- no extra stub `.cc` files are kept

Available imports:
- `Qt` - generic Qt6 helpers (`isAvailable`, `version`, path helpers)
- `Qt/Core` - QtCore helpers and classes (`QtDir`, `QtFile`, `QtProcess`, `QtUrl`, `QtRegExp`, JSON docs)
- `Qt/Gui` - GUI helpers/classes (`QtColor`, `QtFont`, `QtWidget`, `QtPainter`, `QtImage`, filters)
- `Qt/Widgets` - alias of `Qt/Gui` with `QtWidgets` availability module
- `Qt/Application` - alias of `Qt/Gui` exporting `QtApplication` event-loop helpers
- `Qt/Web` - web helpers + `QtWebPage`
- `Qt/WebEngine` - alias of `Qt/Web`
- `Qt/WebView` - alias of `Qt/Web`
- `Qt/WebChannel` - alias of `Qt/Web`
- `Qt/Qml` - QML validation/compile helpers
- `Qt/Quick` - QtQuick item validation helpers
- `Qt/Pdf` - PDF helpers/classes
- `Qt/Xml` - XML helpers/classes

Unified loading API:
- `Qt.load("Core" | "Gui" | "Widgets" | "Application" | "Xml" | "Pdf" | "Web" | "WebEngine" | "WebView" | "WebChannel" | "Qml" | "Quick")`
- `Qt.loadAll()`
- `ImportModule("Qt/All")` to register all Qt submodules at once

Exported globals after registration may include:
- `Qt`, `QtCore`, `QtGui`, `QtWidgets`, `QtApplication`, `QtWeb`, `QtWebEngine`, `QtWebView`, `QtWebChannel`
- `QtQml`, `QtQuick`