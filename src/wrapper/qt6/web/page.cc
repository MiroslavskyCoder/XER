#include "wrapper/qt6/web/page.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QString>
#endif

#if defined(__linux__)
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// WebEngine optional — only if Qt6WebEngineCore found
#if defined(QT_WEBENGINECORE_LIB)
#define HAS_QT_WEBENGINE 1
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QEventLoop>
#include <QTimer>
#else
#define HAS_QT_WEBENGINE 0
#endif

namespace qt6::web {

namespace {

void EnsureLinuxRuntimeDir() {
#if ENGINE_HAS_QT6 && defined(__linux__)
    if (qEnvironmentVariableIsSet("XDG_RUNTIME_DIR")) {
        return;
    }

    const auto user = qEnvironmentVariable("USER");
    QString dir = user.isEmpty()
        ? QString("/tmp/runtime-%1").arg(static_cast<int>(::getuid()))
        : QString("/tmp/runtime-%1").arg(user);

    QDir().mkpath(dir);
    ::chmod(dir.toUtf8().constData(), 0700);
    qputenv("XDG_RUNTIME_DIR", dir.toUtf8());
#endif
}

}  // namespace

bool IsWebEngineAvailable() {
    return HAS_QT_WEBENGINE == 1;
}

std::string GetUserAgent() {
#if HAS_QT_WEBENGINE
    EnsureLinuxRuntimeDir();
    QWebEngineProfile profile;
    return profile.httpUserAgent().toUtf8().constData();
#else
    return "";
#endif
}

std::string GetDefaultCachePath() {
#if ENGINE_HAS_QT6
    EnsureLinuxRuntimeDir();
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
               .toUtf8().constData();
#else
    return "";
#endif
}

std::string GetDefaultStoragePath() {
#if ENGINE_HAS_QT6
    EnsureLinuxRuntimeDir();
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
               .toUtf8().constData();
#else
    return "";
#endif
}

WebRequestResult LoadUrlSync(const std::string& url, int timeout_ms) {
#if HAS_QT_WEBENGINE
    EnsureLinuxRuntimeDir();
    QWebEngineProfile profile;
    QWebEnginePage page(&profile);

    WebRequestResult result;
    bool done = false;

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(timeout_ms);

    QObject::connect(&page, &QWebEnginePage::loadFinished,
        [&](bool ok) {
            result.ok     = ok;
            result.status = ok ? 200 : 0;
            done = true;
            loop.quit();
        });

    QObject::connect(&timeout, &QTimer::timeout, [&]() {
        result.error = "timeout";
        loop.quit();
    });

    page.load(QUrl(QString::fromUtf8(url.c_str())));
    timeout.start();
    loop.exec();

    if (result.ok) {
        // Extract HTML via toHtml (async bridged sync)
        bool html_done = false;
        QEventLoop html_loop;
        page.toHtml([&](const QString& html) {
            result.body = html.toUtf8().constData();
            html_done = true;
            html_loop.quit();
        });
        QTimer::singleShot(5000, &html_loop, &QEventLoop::quit);
        html_loop.exec();
    }

    return result;
#else
    return { false, 0, "", "Qt6::WebEngineCore not available" };
#endif
}

}  // namespace qt6::web
