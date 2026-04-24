#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace openvino {
    namespace runtime {
        class Core;
        class Model;
        class CompiledModel;
        class InferRequest;
    }
}

namespace AsyncIO::IO::DNN {

class VinoEngine {
public:
    VinoEngine();
    ~VinoEngine();

    // Lifecycle management
    bool Initialize(const std::string& model_path, const std::string& device = "CPU");
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

    // Device management
    std::string GetDeviceName() const { return device_name_; }
    bool SetDevice(const std::string& device_name);

    // Error tracking
    std::string GetLastError() const;

private:
    std::unique_ptr<openvino::runtime::Core> core_;
    std::unique_ptr<openvino::runtime::Model> model_;
    std::unique_ptr<openvino::runtime::CompiledModel> compiled_model_;
    std::unique_ptr<openvino::runtime::InferRequest> infer_request_;
    std::vector<std::vector<int64_t>> input_shapes_;
    std::vector<std::vector<int64_t>> output_shapes_;
    std::vector<std::string> input_names_;
    std::vector<std::string> output_names_;
    std::string device_name_;
    std::string last_error_;
    bool initialized_;

    void CacheModelInfo();
};

}  // namespace AsyncIO::IO::DNN
