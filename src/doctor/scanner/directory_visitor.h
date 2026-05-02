#pragma once

#include <string>

namespace EngineDoctor {

class DirectoryVisitor {
public:
    virtual ~DirectoryVisitor() = default;
    virtual void Visit(const std::string& path, int depth) = 0;
};

} // namespace EngineDoctor
