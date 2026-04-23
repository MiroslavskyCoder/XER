#pragma once
#include <string>
#include <vector>
#include "wrapper/qt6/v8/class_builder.h"
#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::core {

struct RegExpMatch {
    bool        matched  = false;
    int         offset   = -1;
    int         length   = 0;
    std::string captured;                          // full match
    std::vector<std::string> groups;               // capture groups
};

class RegExpWrapper {
public:
    explicit RegExpWrapper(const std::string& pattern, bool case_sensitive = true,
                           bool multiline = false, bool dot_all = false);
    ~RegExpWrapper();

    bool isValid()     const;
    std::string errorString() const;
    std::string pattern()     const;

    RegExpMatch    match(const std::string& str, int offset = 0) const;
    std::vector<RegExpMatch> matchAll(const std::string& str) const;

    bool test(const std::string& str) const;

    // Replace first / all occurrences
    std::string replace(const std::string& str, const std::string& replacement) const;
    std::string replaceAll(const std::string& str, const std::string& replacement) const;

    // Split string by pattern
    std::vector<std::string> split(const std::string& str) const;

private:
    void* d_ = nullptr;  // QRegularExpression*
    std::string pattern_;
};

bool RegisterQtRegExpClass(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::core
