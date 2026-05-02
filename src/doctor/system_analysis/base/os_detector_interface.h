#pragma once
#include <string>

namespace EngineDoctor {

enum class OsType { Linux, Windows, MacOS, OpenBSD, Unknown };

class OsDetectorInterface {
public:
    virtual ~OsDetectorInterface() = default;
    virtual OsType Detect() = 0;
    virtual std::string Name() = 0;
};

} // namespace EngineDoctor
