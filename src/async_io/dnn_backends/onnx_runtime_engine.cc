#include "onnx_runtime_engine.h"
 
#include <onnxruntime_cxx_api.h> 

namespace AsyncIO::IO::DNN {

OnnxRuntimeEngine::OnnxRuntimeEngine()
    : initialized_(false) {}

OnnxRuntimeEngine::~OnnxRuntimeEngine() {
    Shutdown();
}

bool OnnxRuntimeEngine::Initialize(const std::string& model_path) { 
    try {
        env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "onnx_engine");
        
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(4);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        session_ = std::make_unique<Ort::Session>(*env_, model_path.c_str(), session_options);
        
        CacheModelInfo();
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("ONNX init failed: ") + e.what();
        return false;
    } 
}

bool OnnxRuntimeEngine::IsInitialized() const {
    return initialized_;
}

void OnnxRuntimeEngine::Shutdown() {
    session_.reset();
    env_.reset();
    initialized_ = false;
}

std::vector<int64_t> OnnxRuntimeEngine::GetInputShape(size_t input_idx) const {
    if (input_idx < input_shapes_.size()) {
        return input_shapes_[input_idx];
    }
    return std::vector<int64_t>();
}

std::vector<int64_t> OnnxRuntimeEngine::GetOutputShape(size_t output_idx) const {
    if (output_idx < output_shapes_.size()) {
        return output_shapes_[output_idx];
    }
    return std::vector<int64_t>();
}

bool OnnxRuntimeEngine::InferenceFloat32(
    const float* input_data,
    size_t input_count,
    float* output_data,
    size_t output_count,
    std::string* error_msg) { 
    if (!initialized_) {
        last_error_ = "Engine not initialized";
        if (error_msg) *error_msg = last_error_;
        return false;
    }

    try {
        // Create input tensor
        auto input_shape = GetInputShape(0);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault),
            const_cast<float*>(input_data),
            input_count,
            input_shape.data(),
            input_shape.size()
        );

        // Run inference
        auto input_names_c = std::vector<const char*>();
        for (const auto& name : input_names_) {
            input_names_c.push_back(name.c_str());
        }
        auto output_names_c = std::vector<const char*>();
        for (const auto& name : output_names_) {
            output_names_c.push_back(name.c_str());
        }

        auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_names_c.data(),
            &input_tensor,
            1,
            output_names_c.data(),
            output_names_c.size()
        );

        // Extract output
        if (!output_tensors.empty()) {
            float* out = output_tensors[0].GetTensorMutableData<float>();
            std::copy(out, out + output_count, output_data);
        }
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Inference failed: ") + e.what();
        if (error_msg) *error_msg = last_error_;
        return false;
    } 
}

void OnnxRuntimeEngine::CacheModelInfo() { 
    if (!session_) return;

    // Cache input shapes and names
    size_t input_count = session_->GetInputCount();
    for (size_t i = 0; i < input_count; ++i) {
        input_names_.push_back(session_->GetInputName(i, Ort::AllocatorWithDefaultOptions()));
        input_shapes_.push_back(session_->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
    }

    // Cache output shapes and names
    size_t output_count = session_->GetOutputCount();
    for (size_t i = 0; i < output_count; ++i) {
        output_names_.push_back(session_->GetOutputName(i, Ort::AllocatorWithDefaultOptions()));
        output_shapes_.push_back(session_->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
    } 
}

std::string OnnxRuntimeEngine::GetLastError() const {
    return last_error_;
}

}  // namespace AsyncIO::IO::DNN
