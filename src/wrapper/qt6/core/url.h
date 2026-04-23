#pragma once
#include <string>
#include "wrapper/qt6/v8/class_builder.h"
#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::core {

class UrlWrapper {
public:
    explicit UrlWrapper(const std::string& url_str = "");
    ~UrlWrapper();

    bool        isValid()    const;
    bool        isEmpty()    const;
    std::string toString()   const;
    std::string scheme()     const;
    std::string host()       const;
    int         port()       const;
    std::string path()       const;
    std::string query()      const;
    std::string fragment()   const;
    std::string userInfo()   const;
    std::string authority()  const;

    void setScheme(const std::string& s);
    void setHost(const std::string& h);
    void setPort(int p);
    void setPath(const std::string& p);
    void setQuery(const std::string& q);
    void setFragment(const std::string& f);

    std::string toLocalFile() const;
    UrlWrapper  resolved(const std::string& relative) const;

    static UrlWrapper fromLocalFile(const std::string& path);

private:
#if ENGINE_HAS_QT6
    void* d_ = nullptr;  // opaque QUrl*
#else
    std::string raw_;
#endif
};

bool RegisterQtUrlClass(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::core
