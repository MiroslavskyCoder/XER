#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "wrapper/qt6/v8/class_builder.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::core {

// ---------------------------------------------------------------------------
// FileWrapper — C++ side of QtFile JS class
// Sync operations via QFile; async via AsyncFileReader / AsyncFileWriter
// ---------------------------------------------------------------------------
class FileWrapper {
public:
    explicit FileWrapper(const std::string& path);
    ~FileWrapper();

    const std::string& filePath() const;
    bool   exists()    const;
    int64_t size()     const;
    bool   isSymLink() const;

    // Sync read → string (UTF-8)
    bool   readText(std::string& out, std::string& err) const;

    // Sync read → binary bytes
    bool   readBytes(std::vector<uint8_t>& out, std::string& err) const;

    // Sync write (overwrite)
    bool   writeText(const std::string& text, std::string& err) const;
    bool   writeBytes(const std::vector<uint8_t>& data, std::string& err) const;

    // Sync append
    bool   appendText(const std::string& text, std::string& err) const;

    // File management
    bool   remove(std::string& err) const;
    bool   rename(const std::string& new_path, std::string& err) const;
    bool   copy(const std::string& dst_path, std::string& err) const;

    // Meta
    std::string absolutePath() const;
    std::string dirPath()      const;
    std::string baseName()     const;
    std::string extension()    const;
    std::string mimeType()     const;  // via QMimeDatabase

private:
    std::string path_;
};

// ---------------------------------------------------------------------------
// V8 registration
// ---------------------------------------------------------------------------
bool RegisterQtFileClass(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::core
