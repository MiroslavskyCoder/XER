#include "wrapper/qt6/web/web_page_runtime.h"

#if HAS_QT_WEBENGINE
#include <QDir>

#if defined(__linux__)
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace qt6::web {

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

WebPageWrapper::WebPageWrapper() {
    EnsureLinuxRuntimeDir();
    profile_ = std::make_unique<QWebEngineProfile>();
    page_ = std::make_unique<QWebEnginePage>(profile_.get());
}

QWebEngineProfile* WebPageWrapper::profile() const {
    return profile_.get();
}

QWebEnginePage* WebPageWrapper::page() const {
    return page_.get();
}

}  // namespace qt6::web
#endif
