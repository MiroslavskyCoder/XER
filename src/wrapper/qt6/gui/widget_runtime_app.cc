#include "wrapper/qt6/gui/widget_runtime.h"

#if defined(__linux__)
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace qt6::gui {

#if HAS_QT_WIDGETS
void EnsureLinuxRuntimeDir() {
#if defined(__linux__)
    if (qEnvironmentVariableIsSet("XDG_RUNTIME_DIR")) {
        return;
    }

    const auto user = qEnvironmentVariable("USER");
    QString dir = user.isEmpty() ? QString("/tmp/runtime-%1").arg(static_cast<int>(::getuid()))
                                 : QString("/tmp/runtime-%1").arg(user);

    QDir().mkpath(dir);
    ::chmod(dir.toUtf8().constData(), 0700);
    qputenv("XDG_RUNTIME_DIR", dir.toUtf8());
#endif
}

QApplication* EnsureGuiApp() {
    EnsureLinuxRuntimeDir();
    if (auto* app = qobject_cast<QApplication*>(QCoreApplication::instance())) {
        return app;
    }

    static int argc = 1;
    static char app_name[] = "EngineBuilder";
    static char* argv[] = {app_name, nullptr};
    static QApplication* created = new QApplication(argc, argv);
    return created;
}

WidgetWrapper::WidgetWrapper(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    const std::string& title)
    : isolate_(isolate) {
    EnsureGuiApp();
    context_.Reset(isolate, context);
    widget_ = std::make_unique<QWidget>();
    if (!title.empty()) {
        widget_->setWindowTitle(QString::fromUtf8(title.c_str()));
    }
    widget_->resize(640, 360);
}

WidgetWrapper::~WidgetWrapper() {
    for (auto& [_, cb] : click_callbacks_) {
        cb.Reset();
    }
    for (auto& [_, cb] : text_callbacks_) {
        cb.Reset();
    }
    context_.Reset();
}

QWidget* WidgetWrapper::get() const {
    return widget_.get();
}
#endif

}  // namespace qt6::gui
