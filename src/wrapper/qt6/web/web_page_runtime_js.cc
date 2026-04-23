#include "wrapper/qt6/web/web_page_runtime.h"

#if HAS_QT_WEBENGINE
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QVariant>

namespace qt6::web {

std::string RunJsSync(QWebEnginePage* page, const std::string& script, int timeout_ms) {
    std::string out;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);

    QObject::connect(&timer, &QTimer::timeout, [&]() { loop.quit(); });

    page->runJavaScript(QString::fromUtf8(script.c_str()), [&](const QVariant& value) {
        if (value.typeId() == QMetaType::QString) {
            out = value.toString().toUtf8().constData();
        } else if (value.canConvert<QJsonObject>()) {
            out = QJsonDocument(value.toJsonObject()).toJson(QJsonDocument::Compact).toStdString();
        } else {
            out = value.toString().toUtf8().constData();
        }
        loop.quit();
    });

    timer.start();
    loop.exec();
    return out;
}

}  // namespace qt6::web
#endif
