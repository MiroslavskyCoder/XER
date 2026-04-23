#pragma once

#include <string>
#include <vector>

#include "wrapper/qt6/v8/class_builder.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QDir>
#endif

namespace qt6::core {

// ---------------------------------------------------------------------------
// DirWrapper — C++ side of QtDir JS class
// ---------------------------------------------------------------------------
class DirWrapper {
public:
    explicit DirWrapper(const std::string& path = ".");

    std::string      path() const;
    std::string      absolutePath() const;
    std::string      dirName() const;
    std::string      canonicalPath() const;
    bool             exists() const;
    bool             exists(const std::string& name) const;
    bool             isRoot() const;
    bool             isRelative() const;
    bool             isAbsolute() const;
    bool             isReadable() const;

    bool             cd(const std::string& dir_name);
    bool             cdUp();
    bool             mkdir(const std::string& dir_name) const;
    bool             mkpath(const std::string& dir_path) const;
    bool             rmdir(const std::string& dir_name) const;
    bool             removeRecursively();
    bool             rename(const std::string& old_name, const std::string& new_name);
    bool             remove(const std::string& file_name);

    std::vector<std::string> entryList(const std::string& filter = "*",
                                        bool include_dirs = true,
                                        bool include_hidden = false) const;

    std::vector<std::string> entryListDirs(bool include_hidden = false) const;
    std::vector<std::string> entryListFiles(const std::string& filter = "*") const;

    // Static helpers exposed on the class itself
    static std::string homePath();
    static std::string currentPath();
    static std::string tempPath();
    static bool        setCurrent(const std::string& path);
    static std::vector<std::string> drives();

private:
#if ENGINE_HAS_QT6
    QDir dir_;
#else
    std::string path_;
#endif
};

// ---------------------------------------------------------------------------
// V8 registration (called by qt_core_module)
// ---------------------------------------------------------------------------
bool RegisterQtDirClass(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::core
