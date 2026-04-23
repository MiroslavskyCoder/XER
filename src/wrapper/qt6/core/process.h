#pragma once

#include <string>
#include <vector>

#include "wrapper/qt6/v8/class_builder.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::core {

struct ProcessResult {
    int         exit_code  = -1;
    std::string stdout_out;
    std::string stderr_out;
    std::string error;      // launch/timeout error
    bool        ok         = false;
};

class ProcessWrapper {
public:
    ProcessWrapper();
    ~ProcessWrapper();

    // Synchronous run — blocks until finished or timeout_ms exceeded
    ProcessResult run(const std::string& program,
                      const std::vector<std::string>& args,
                      int timeout_ms = 30000) const;

    // Working directory
    void setWorkDir(const std::string& dir);
    const std::string& workDir() const;

    // Environment key/value
    void setEnv(const std::string& key, const std::string& value);
    void clearEnv();

private:
    std::string work_dir_;
    std::vector<std::pair<std::string, std::string>> env_;
};

bool RegisterQtProcessClass(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::core
