#include "wrapper/qt6/core/path.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QtGlobal>
#endif

namespace qt6::core {

bool IsAvailable() {
    return ENGINE_HAS_QT6 == 1;
}

std::string VersionString() {
#if ENGINE_HAS_QT6
    return qVersion();
#else
    return "unavailable";
#endif
}

std::string CleanPath(const std::string& value) {
#if ENGINE_HAS_QT6
    return QDir::cleanPath(QString::fromUtf8(value.c_str())).toUtf8().constData();
#else
    return value;
#endif
}

std::string ToNativeSeparators(const std::string& value) {
#if ENGINE_HAS_QT6
    return QDir::toNativeSeparators(QString::fromUtf8(value.c_str())).toUtf8().constData();
#else
    return value;
#endif
}

std::string AbsolutePath(const std::string& value) {
#if ENGINE_HAS_QT6
    return QDir(QString::fromUtf8(value.c_str())).absolutePath().toUtf8().constData();
#else
    return value;
#endif
}

std::string DirName(const std::string& value) {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(value.c_str())).dir().path().toUtf8().constData();
#else
    const auto pos = value.find_last_of("/\\");
    return pos == std::string::npos ? "." : value.substr(0, pos);
#endif
}

std::string BaseName(const std::string& value) {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(value.c_str())).fileName().toUtf8().constData();
#else
    const auto pos = value.find_last_of("/\\");
    return pos == std::string::npos ? value : value.substr(pos + 1);
#endif
}

std::string Extension(const std::string& value) {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(value.c_str())).suffix().toUtf8().constData();
#else
    const auto pos = value.rfind('.');
    return pos == std::string::npos ? "" : value.substr(pos + 1);
#endif
}

std::string JoinPath(const std::string& base, const std::string& child) {
#if ENGINE_HAS_QT6
    return QDir(QString::fromUtf8(base.c_str()))
               .filePath(QString::fromUtf8(child.c_str()))
               .toUtf8().constData();
#else
    if (base.empty()) return child;
    if (base.back() == '/' || base.back() == '\\') return base + child;
    return base + '/' + child;
#endif
}

bool PathExists(const std::string& value) {
#if ENGINE_HAS_QT6
    return QFileInfo::exists(QString::fromUtf8(value.c_str()));
#else
    return false;
#endif
}

bool IsDir(const std::string& value) {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(value.c_str())).isDir();
#else
    return false;
#endif
}

}  // namespace qt6::core