#pragma once

#include <string>

namespace Engine::ModelsBuilder::Reader::Onnx {

/// Checks ONNX opset / IR version compatibility.
class OnnxVersionChecker {
public:
    static constexpr int kMaxSupportedOpset = 17;
    static constexpr int kMinSupportedOpset = 1;

    /// Returns true if opset_version is within the supported range.
    static bool IsOpsetSupported(int opset_version) noexcept;

    /// Returns a human-readable string describing version support.
    static std::string Describe(int opset_version);
};

}  // namespace Engine::ModelsBuilder::Reader::Onnx
