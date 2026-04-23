#include "wrapper/qt6/web/web_page_runtime.h"

#if HAS_QT_WEBENGINE
#include <QEventLoop>
#include <QTimer>
#include <QUrl>

namespace qt6::web {

WebRequestResult LoadOnPageSync(QWebEnginePage* page, const std::string& url, int timeout_ms) {
    WebRequestResult result;
    QEventLoop loop;

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);

    QObject::connect(&timer, &QTimer::timeout, [&]() {
        result.error = "timeout";
        loop.quit();
    });

    QObject::connect(page, &QWebEnginePage::loadFinished, [&](bool ok) {
        result.ok = ok;
        result.status = ok ? 200 : 0;
        loop.quit();
    });

    page->load(QUrl(QString::fromUtf8(url.c_str())));
    timer.start();
    loop.exec();

    if (!result.ok) {
        if (result.error.empty()) {
            result.error = "load failed";
        }
        return result;
    }

    QEventLoop html_loop;
    QTimer html_timer;
    html_timer.setSingleShot(true);
    html_timer.setInterval(timeout_ms);

    QObject::connect(&html_timer, &QTimer::timeout, [&]() {
        if (result.body.empty()) {
            result.error = "html extraction timeout";
        }
        html_loop.quit();
    });

    page->toHtml([&](const QString& html) {
        result.body = html.toUtf8().constData();
        html_loop.quit();
    });

    html_timer.start();
    html_loop.exec();
    return result;
}

std::string PageToPlainText(QWebEnginePage* page, int timeout_ms) {
    std::string out;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);

    QObject::connect(&timer, &QTimer::timeout, [&]() { loop.quit(); });
    page->toPlainText([&](const QString& text) {
        out = text.toUtf8().constData();
        loop.quit();
    });

    timer.start();
    loop.exec();
    return out;
}

std::string PageToHtml(QWebEnginePage* page, int timeout_ms) {
    std::string out;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);

    QObject::connect(&timer, &QTimer::timeout, [&]() { loop.quit(); });
    page->toHtml([&](const QString& html) {
        out = html.toUtf8().constData();
        loop.quit();
    });

    timer.start();
    loop.exec();
    return out;
}

}  // namespace qt6::web
#endif
