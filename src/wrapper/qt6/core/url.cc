#include "wrapper/qt6/core/url.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QUrl>
#include <QString>
#endif

namespace qt6::core {

// ---------------------------------------------------------------------------
// UrlWrapper — stores QUrl* in d_ (avoids including QUrl in header)
// ---------------------------------------------------------------------------

#if ENGINE_HAS_QT6
#define QU() (*reinterpret_cast<QUrl*>(d_))
UrlWrapper::UrlWrapper(const std::string& s)
    : d_(new QUrl(QString::fromUtf8(s.c_str()))) {}

UrlWrapper::~UrlWrapper() { delete reinterpret_cast<QUrl*>(d_); }

bool UrlWrapper::isValid()   const { return QU().isValid(); }
bool UrlWrapper::isEmpty()   const { return QU().isEmpty(); }
std::string UrlWrapper::toString()   const { return QU().toString().toUtf8().constData(); }
std::string UrlWrapper::scheme()     const { return QU().scheme().toUtf8().constData(); }
std::string UrlWrapper::host()       const { return QU().host().toUtf8().constData(); }
int         UrlWrapper::port()       const { return QU().port(); }
std::string UrlWrapper::path()       const { return QU().path().toUtf8().constData(); }
std::string UrlWrapper::query()      const { return QU().query().toUtf8().constData(); }
std::string UrlWrapper::fragment()   const { return QU().fragment().toUtf8().constData(); }
std::string UrlWrapper::userInfo()   const { return QU().userInfo().toUtf8().constData(); }
std::string UrlWrapper::authority()  const { return QU().authority().toUtf8().constData(); }
std::string UrlWrapper::toLocalFile() const { return QU().toLocalFile().toUtf8().constData(); }
void UrlWrapper::setScheme(const std::string& s) { QU().setScheme(QString::fromUtf8(s.c_str())); }
void UrlWrapper::setHost(const std::string& h)   { QU().setHost(QString::fromUtf8(h.c_str())); }
void UrlWrapper::setPort(int p)                  { QU().setPort(p); }
void UrlWrapper::setPath(const std::string& p)   { QU().setPath(QString::fromUtf8(p.c_str())); }
void UrlWrapper::setQuery(const std::string& q)  { QU().setQuery(QString::fromUtf8(q.c_str())); }
void UrlWrapper::setFragment(const std::string& f) { QU().setFragment(QString::fromUtf8(f.c_str())); }
UrlWrapper UrlWrapper::resolved(const std::string& rel) const {
    QUrl r = QU().resolved(QUrl(QString::fromUtf8(rel.c_str())));
    return UrlWrapper(r.toString().toUtf8().constData());
}
UrlWrapper UrlWrapper::fromLocalFile(const std::string& path) {
    return UrlWrapper(QUrl::fromLocalFile(
        QString::fromUtf8(path.c_str())).toString().toUtf8().constData());
}
#undef QU
#else
// Stub
UrlWrapper::UrlWrapper(const std::string& s) : raw_(s) {}
UrlWrapper::~UrlWrapper() = default;
bool UrlWrapper::isValid()   const { return !raw_.empty(); }
bool UrlWrapper::isEmpty()   const { return raw_.empty(); }
std::string UrlWrapper::toString()   const { return raw_; }
std::string UrlWrapper::scheme()     const { return ""; }
std::string UrlWrapper::host()       const { return ""; }
int         UrlWrapper::port()       const { return -1; }
std::string UrlWrapper::path()       const { return raw_; }
std::string UrlWrapper::query()      const { return ""; }
std::string UrlWrapper::fragment()   const { return ""; }
std::string UrlWrapper::userInfo()   const { return ""; }
std::string UrlWrapper::authority()  const { return ""; }
std::string UrlWrapper::toLocalFile() const { return raw_; }
void UrlWrapper::setScheme(const std::string&) {}
void UrlWrapper::setHost(const std::string&)   {}
void UrlWrapper::setPort(int)                  {}
void UrlWrapper::setPath(const std::string& p) { raw_ = p; }
void UrlWrapper::setQuery(const std::string&)  {}
void UrlWrapper::setFragment(const std::string&) {}
UrlWrapper UrlWrapper::resolved(const std::string& rel) const { return UrlWrapper(rel); }
UrlWrapper UrlWrapper::fromLocalFile(const std::string& p) { return UrlWrapper("file://" + p); }
#endif

}  // namespace qt6::core
