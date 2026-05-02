#pragma once

#include <string>

namespace EngineDoctor {

class FileVisitor {
public:
    virtual ~FileVisitor() = default;
    virtual void Visit(const std::string& path) = 0;
};

} // namespace EngineDoctor
