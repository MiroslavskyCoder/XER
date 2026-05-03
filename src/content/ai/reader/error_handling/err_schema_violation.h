#pragma once

#include <stdexcept>
#include <string>

namespace Engine::ModelsBuilder::Reader {

/// Thrown when parsed model data violates the expected schema
/// (e.g. missing required field, wrong tensor rank, unsupported attribute type).
class SchemaViolationError : public std::runtime_error {
public:
    explicit SchemaViolationError(const std::string& msg)
        : std::runtime_error("[SchemaViolation] " + msg) {}

    SchemaViolationError(const std::string& field, const std::string& detail)
        : std::runtime_error("[SchemaViolation] field='" + field + "': " + detail) {}
};

}  // namespace Engine::ModelsBuilder::Reader
