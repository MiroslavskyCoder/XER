#pragma once

#include "wrapper/qt6/web/page.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6 && defined(QT_WEBENGINECORE_LIB)
#define HAS_QT_WEBENGINE 1
#include <QWebEnginePage>
#include <QWebEngineProfile>
#else
#define HAS_QT_WEBENGINE 0
#endif

#include <memory>
#include <string>

namespace qt6::web {

#if HAS_QT_WEBENGINE
void EnsureLinuxRuntimeDir();

class WebPageWrapper {
public:
    WebPageWrapper();

    QWebEngineProfile* profile() const;
    QWebEnginePage* page() const;

private:
    std::unique_ptr<QWebEngineProfile> profile_;
    std::unique_ptr<QWebEnginePage> page_;
};

WebRequestResult LoadOnPageSync(QWebEnginePage* page, const std::string& url, int timeout_ms);
std::string PageToPlainText(QWebEnginePage* page, int timeout_ms);
std::string PageToHtml(QWebEnginePage* page, int timeout_ms);
std::string RunJsSync(QWebEnginePage* page, const std::string& script, int timeout_ms);
#endif

}  // namespace qt6::web
