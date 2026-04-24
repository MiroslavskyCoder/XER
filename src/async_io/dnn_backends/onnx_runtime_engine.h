#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

// ONNX Runtime forward declarations
namespace Ort {
    struct Session;
    struct Env;
    class Value;
}

namespace AsyncIO::IO::DNN {

class OnnxRuntimeEngine {
public:
    OnnxRuntimeEngine();
    ~OnnxRuntimeEngine();

    // Lifecycle management
    bool Initialize(const std::string& model_path);
    bool IsInitialized() const;
    void Shutdown();

    // Model information
    std::vector<int64_t> GetInputShape(size_t input_idx = 0) const;
    std::vector<int64_t> GetOutputShape(size_t output_idx = 0) const;
    size_t GetInputCount() const { return input_shapes_.size(); }
    size_t GetOutputCount() const { return output_shapes_.size(); }

    // Inference
    bool InferenceFloat32(
        const float* input_data,
        size_t input_count,
        float* output_data,
        size_t output_count,
        std::string* error_msg = nullptr
    );

    // Error tracking
    std::string GetLastError() const;

private:
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::vector<std::vector<int64_t>> input_shapes_;
    std::vector<std::vector<int64_t>> output_shapes_;
    std::vector<std::string> input_names_;
    std::vector<std::string> output_names_;
    std::string last_error_;
    bool initialized_;

    void CacheModelInfo();
};

}  // namespace AsyncIO::IO::DNN
