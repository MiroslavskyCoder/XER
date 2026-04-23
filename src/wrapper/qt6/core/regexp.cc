#include "wrapper/qt6/core/regexp.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QString>
#endif

namespace qt6::core {

#if ENGINE_HAS_QT6
#define QRE() (*reinterpret_cast<QRegularExpression*>(d_))

RegExpWrapper::RegExpWrapper(const std::string& pattern, bool case_sensitive,
                              bool multiline, bool dot_all)
    : pattern_(pattern)
{
    QRegularExpression::PatternOptions opts;
    if (!case_sensitive) opts |= QRegularExpression::CaseInsensitiveOption;
    if (multiline)       opts |= QRegularExpression::MultilineOption;
    if (dot_all)         opts |= QRegularExpression::DotMatchesEverythingOption;
    d_ = new QRegularExpression(QString::fromUtf8(pattern.c_str()), opts);
}
RegExpWrapper::~RegExpWrapper() { delete reinterpret_cast<QRegularExpression*>(d_); }

bool RegExpWrapper::isValid()  const { return QRE().isValid(); }
std::string RegExpWrapper::errorString() const {
    return QRE().errorString().toUtf8().constData();
}
std::string RegExpWrapper::pattern() const { return pattern_; }

RegExpMatch RegExpWrapper::match(const std::string& str, int offset) const {
    RegExpMatch rm;
    auto m = QRE().match(QString::fromUtf8(str.c_str()), offset);
    rm.matched  = m.hasMatch();
    rm.offset   = m.capturedStart(0);
    rm.length   = m.capturedLength(0);
    rm.captured = m.captured(0).toUtf8().constData();
    for (int i = 1; i <= QRE().captureCount(); ++i)
        rm.groups.push_back(m.captured(i).toUtf8().constData());
    return rm;
}

std::vector<RegExpMatch> RegExpWrapper::matchAll(const std::string& str) const {
    std::vector<RegExpMatch> out;
    auto it = QRE().globalMatch(QString::fromUtf8(str.c_str()));
    while (it.hasNext()) {
        auto m = it.next();
        RegExpMatch rm;
        rm.matched  = true;
        rm.offset   = m.capturedStart(0);
        rm.length   = m.capturedLength(0);
        rm.captured = m.captured(0).toUtf8().constData();
        for (int i = 1; i <= QRE().captureCount(); ++i)
            rm.groups.push_back(m.captured(i).toUtf8().constData());
        out.push_back(std::move(rm));
    }
    return out;
}

bool RegExpWrapper::test(const std::string& str) const {
    return QRE().match(QString::fromUtf8(str.c_str())).hasMatch();
}

std::string RegExpWrapper::replace(const std::string& str, const std::string& repl) const {
    QString qs = QString::fromUtf8(str.c_str());
    return qs.replace(QRE(), QString::fromUtf8(repl.c_str())).toUtf8().constData();
    // Note: QString::replace uses the first match only for QRegularExpression — use replaceAll for all
}

std::string RegExpWrapper::replaceAll(const std::string& str, const std::string& repl) const {
    // QRegularExpression replaces all matches by default with QString::replace
    QString qs = QString::fromUtf8(str.c_str());
    return qs.replace(QRE(), QString::fromUtf8(repl.c_str())).toUtf8().constData();
}

std::vector<std::string> RegExpWrapper::split(const std::string& str) const {
    QStringList parts = QString::fromUtf8(str.c_str()).split(QRE());
    std::vector<std::string> out;
    for (const auto& p : parts) out.push_back(p.toUtf8().constData());
    return out;
}
#undef QRE

#else
RegExpWrapper::RegExpWrapper(const std::string& p, bool, bool, bool) : pattern_(p) {}
RegExpWrapper::~RegExpWrapper() = default;
bool RegExpWrapper::isValid() const { return false; }
std::string RegExpWrapper::errorString() const { return "Qt6 unavailable"; }
std::string RegExpWrapper::pattern() const { return pattern_; }
RegExpMatch RegExpWrapper::match(const std::string&, int) const { return {}; }
std::vector<RegExpMatch> RegExpWrapper::matchAll(const std::string&) const { return {}; }
bool RegExpWrapper::test(const std::string&) const { return false; }
std::string RegExpWrapper::replace(const std::string& s, const std::string&) const { return s; }
std::string RegExpWrapper::replaceAll(const std::string& s, const std::string&) const { return s; }
std::vector<std::string> RegExpWrapper::split(const std::string& s) const { return {s}; }
#endif

}  // namespace qt6::core
