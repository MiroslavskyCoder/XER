#include "wrapper/qt6/core/file.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QString>
#endif

namespace qt6::core {

// ---------------------------------------------------------------------------
// FileWrapper implementation
// ---------------------------------------------------------------------------
FileWrapper::FileWrapper(const std::string& path) : path_(path) {}
FileWrapper::~FileWrapper() = default;

const std::string& FileWrapper::filePath() const { return path_; }

bool FileWrapper::exists() const {
#if ENGINE_HAS_QT6
    return QFileInfo::exists(QString::fromUtf8(path_.c_str()));
#else
    return false;
#endif
}

int64_t FileWrapper::size() const {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(path_.c_str())).size();
#else
    return -1;
#endif
}

bool FileWrapper::isSymLink() const {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(path_.c_str())).isSymLink();
#else
    return false;
#endif
}

std::string FileWrapper::absolutePath() const {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(path_.c_str())).absoluteFilePath().toUtf8().constData();
#else
    return path_;
#endif
}

std::string FileWrapper::dirPath() const {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(path_.c_str())).absolutePath().toUtf8().constData();
#else
    auto pos = path_.rfind('/');
    return pos == std::string::npos ? "." : path_.substr(0, pos);
#endif
}

std::string FileWrapper::baseName() const {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(path_.c_str())).fileName().toUtf8().constData();
#else
    auto pos = path_.rfind('/');
    return pos == std::string::npos ? path_ : path_.substr(pos + 1);
#endif
}

std::string FileWrapper::extension() const {
#if ENGINE_HAS_QT6
    return QFileInfo(QString::fromUtf8(path_.c_str())).suffix().toUtf8().constData();
#else
    auto pos = path_.rfind('.');
    return pos == std::string::npos ? "" : path_.substr(pos + 1);
#endif
}

std::string FileWrapper::mimeType() const {
#if ENGINE_HAS_QT6
    QMimeDatabase db;
    return db.mimeTypeForFile(QString::fromUtf8(path_.c_str())).name().toUtf8().constData();
#else
    return "";
#endif
}

bool FileWrapper::readText(std::string& out, std::string& err) const {
#if ENGINE_HAS_QT6
    QFile f(QString::fromUtf8(path_.c_str()));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        err = f.errorString().toUtf8().constData();
        return false;
    }
    out = f.readAll().toStdString();
    return true;
#else
    err = "Qt6 unavailable";
    return false;
#endif
}

bool FileWrapper::readBytes(std::vector<uint8_t>& out, std::string& err) const {
#if ENGINE_HAS_QT6
    QFile f(QString::fromUtf8(path_.c_str()));
    if (!f.open(QIODevice::ReadOnly)) {
        err = f.errorString().toUtf8().constData();
        return false;
    }
    QByteArray ba = f.readAll();
    out.assign(reinterpret_cast<const uint8_t*>(ba.constData()),
               reinterpret_cast<const uint8_t*>(ba.constData()) + ba.size());
    return true;
#else
    err = "Qt6 unavailable";
    return false;
#endif
}

bool FileWrapper::writeText(const std::string& text, std::string& err) const {
#if ENGINE_HAS_QT6
    QFile f(QString::fromUtf8(path_.c_str()));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        err = f.errorString().toUtf8().constData();
        return false;
    }
    qint64 written = f.write(text.c_str(), static_cast<qint64>(text.size()));
    return written == static_cast<qint64>(text.size());
#else
    err = "Qt6 unavailable";
    return false;
#endif
}

bool FileWrapper::writeBytes(const std::vector<uint8_t>& data, std::string& err) const {
#if ENGINE_HAS_QT6
    QFile f(QString::fromUtf8(path_.c_str()));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        err = f.errorString().toUtf8().constData();
        return false;
    }
    qint64 written = f.write(reinterpret_cast<const char*>(data.data()),
                              static_cast<qint64>(data.size()));
    return written == static_cast<qint64>(data.size());
#else
    err = "Qt6 unavailable";
    return false;
#endif
}

bool FileWrapper::appendText(const std::string& text, std::string& err) const {
#if ENGINE_HAS_QT6
    QFile f(QString::fromUtf8(path_.c_str()));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        err = f.errorString().toUtf8().constData();
        return false;
    }
    f.write(text.c_str(), static_cast<qint64>(text.size()));
    return true;
#else
    err = "Qt6 unavailable";
    return false;
#endif
}

bool FileWrapper::remove(std::string& err) const {
#if ENGINE_HAS_QT6
    QFile f(QString::fromUtf8(path_.c_str()));
    if (!f.remove()) { err = f.errorString().toUtf8().constData(); return false; }
    return true;
#else
    err = "Qt6 unavailable"; return false;
#endif
}

bool FileWrapper::rename(const std::string& new_path, std::string& err) const {
#if ENGINE_HAS_QT6
    QFile f(QString::fromUtf8(path_.c_str()));
    if (!f.rename(QString::fromUtf8(new_path.c_str()))) {
        err = f.errorString().toUtf8().constData();
        return false;
    }
    return true;
#else
    err = "Qt6 unavailable"; return false;
#endif
}

bool FileWrapper::copy(const std::string& dst, std::string& err) const {
#if ENGINE_HAS_QT6
    if (!QFile::copy(QString::fromUtf8(path_.c_str()),
                     QString::fromUtf8(dst.c_str()))) {
        err = "Copy failed";
        return false;
    }
    return true;
#else
    err = "Qt6 unavailable"; return false;
#endif
}

}  // namespace qt6::core
