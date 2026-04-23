#include "wrapper/qt6/core/dir.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QDir>
#include <QFileInfoList>
#include <QString>
#include <QStorageInfo>
#endif

namespace qt6::core {

// ---------------------------------------------------------------------------
// DirWrapper implementation
// ---------------------------------------------------------------------------

DirWrapper::DirWrapper(const std::string& path)
#if ENGINE_HAS_QT6
    : dir_(QString::fromUtf8(path.c_str()))
#else
    : path_(path)
#endif
{}

#if ENGINE_HAS_QT6
#define D dir_
#define QS(s) QString::fromUtf8((s).c_str())
#define SQ(q) (q).toUtf8().constData()

std::string DirWrapper::path()          const { return SQ(D.path()); }
std::string DirWrapper::absolutePath()  const { return SQ(D.absolutePath()); }
std::string DirWrapper::dirName()       const { return SQ(D.dirName()); }
std::string DirWrapper::canonicalPath() const { return SQ(D.canonicalPath()); }
bool DirWrapper::exists()               const { return D.exists(); }
bool DirWrapper::exists(const std::string& n) const { return D.exists(QS(n)); }
bool DirWrapper::isRoot()               const { return D.isRoot(); }
bool DirWrapper::isRelative()           const { return D.isRelative(); }
bool DirWrapper::isAbsolute()           const { return D.isAbsolute(); }
bool DirWrapper::isReadable()           const { return D.isReadable(); }
bool DirWrapper::cd(const std::string& n)    { return D.cd(QS(n)); }
bool DirWrapper::cdUp()                      { return D.cdUp(); }
bool DirWrapper::mkdir(const std::string& n) const { return D.mkdir(QS(n)); }
bool DirWrapper::mkpath(const std::string& p) const { return D.mkpath(QS(p)); }
bool DirWrapper::rmdir(const std::string& n) const { return D.rmdir(QS(n)); }
bool DirWrapper::removeRecursively() { return D.removeRecursively(); }
bool DirWrapper::rename(const std::string& o, const std::string& n) { return D.rename(QS(o), QS(n)); }
bool DirWrapper::remove(const std::string& n) { return D.remove(QS(n)); }

std::vector<std::string> DirWrapper::entryList(const std::string& filter,
                                                 bool include_dirs,
                                                 bool include_hidden) const {
    QDir::Filters f = QDir::Files;
    if (include_dirs)   f |= QDir::Dirs | QDir::NoDotAndDotDot;
    if (include_hidden) f |= QDir::Hidden;
    QStringList lst = D.entryList({ QS(filter) }, f, QDir::Name);
    std::vector<std::string> out;
    out.reserve(lst.size());
    for (const auto& s : lst) out.push_back(SQ(s));
    return out;
}

std::vector<std::string> DirWrapper::entryListDirs(bool include_hidden) const {
    QDir::Filters f = QDir::Dirs | QDir::NoDotAndDotDot;
    if (include_hidden) f |= QDir::Hidden;
    QStringList lst = D.entryList(f, QDir::Name);
    std::vector<std::string> out;
    for (const auto& s : lst) out.push_back(SQ(s));
    return out;
}

std::vector<std::string> DirWrapper::entryListFiles(const std::string& filter) const {
    QStringList lst = D.entryList({ QS(filter) }, QDir::Files, QDir::Name);
    std::vector<std::string> out;
    for (const auto& s : lst) out.push_back(SQ(s));
    return out;
}

std::string DirWrapper::homePath()    { return SQ(QDir::homePath()); }
std::string DirWrapper::currentPath() { return SQ(QDir::currentPath()); }
std::string DirWrapper::tempPath()    { return SQ(QDir::tempPath()); }
bool DirWrapper::setCurrent(const std::string& p) { return QDir::setCurrent(QS(p)); }

std::vector<std::string> DirWrapper::drives() {
    std::vector<std::string> out;
    for (const auto& fi : QDir::drives())
        out.push_back(SQ(fi.absoluteFilePath()));
    return out;
}

#undef D
#undef QS
#undef SQ

#else
// Stub implementations when Qt6 not available
std::string DirWrapper::path()          const { return path_; }
std::string DirWrapper::absolutePath()  const { return path_; }
std::string DirWrapper::dirName()       const { return ""; }
std::string DirWrapper::canonicalPath() const { return path_; }
bool DirWrapper::exists()               const { return false; }
bool DirWrapper::exists(const std::string&) const { return false; }
bool DirWrapper::isRoot()               const { return false; }
bool DirWrapper::isRelative()           const { return true; }
bool DirWrapper::isAbsolute()           const { return false; }
bool DirWrapper::isReadable()           const { return false; }
bool DirWrapper::cd(const std::string& n)     { path_ += "/" + n; return false; }
bool DirWrapper::cdUp()                       { return false; }
bool DirWrapper::mkdir(const std::string&) const { return false; }
bool DirWrapper::mkpath(const std::string&) const { return false; }
bool DirWrapper::rmdir(const std::string&) const { return false; }
bool DirWrapper::removeRecursively()    const { return false; }
bool DirWrapper::rename(const std::string&, const std::string&) { return false; }
bool DirWrapper::remove(const std::string&) const { return false; }
std::vector<std::string> DirWrapper::entryList(const std::string&, bool, bool) const { return {}; }
std::vector<std::string> DirWrapper::entryListDirs(bool) const { return {}; }
std::vector<std::string> DirWrapper::entryListFiles(const std::string&) const { return {}; }
std::string DirWrapper::homePath()    { return ""; }
std::string DirWrapper::currentPath() { return ""; }
std::string DirWrapper::tempPath()    { return ""; }
bool DirWrapper::setCurrent(const std::string&) { return false; }
std::vector<std::string> DirWrapper::drives()  { return {}; }
#endif

}  // namespace qt6::core
