#include "vino_engine.h"
 
#include <openvino/openvino.hpp> 

namespace AsyncIO::IO::DNN {

VinoEngine::VinoEngine()
    : device_name_("CPU"), initialized_(false) {}

VinoEngine::~VinoEngine() {
    Shutdown();
}

bool VinoEngine::Initialize(const std::string& model_path, const std::string& device) { 
    try {
        core_ = std::make_unique<ov::Core>();
        device_name_ = device;

        // Read model
        model_ = core_->read_model(model_path);

        // Compile model
        compiled_model_ = core_->compile_model(model_, device);

        // Create inference request
        infer_request_ = compiled_model_->create_infer_request();

        CacheModelInfo();
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("OpenVINO init failed: ") + e.what();
        return false;
    } 
}

bool VinoEngine::IsInitialized() const {
    return initialized_;
}

void VinoEngine::Shutdown() {
    infer_request_.reset();
    compiled_model_.reset();
    model_.reset();
    core_.reset();
    initialized_ = false;
}

std::vector<int64_t> VinoEngine::GetInputShape(size_t input_idx) const {
    if (input_idx < input_shapes_.size()) {
        return input_shapes_[input_idx];
    }
    return std::vector<int64_t>();
}

std::vector<int64_t> VinoEngine::GetOutputShape(size_t output_idx) const {
    if (output_idx < output_shapes_.size()) {
        return output_shapes_[output_idx];
    }
    return std::vector<int64_t>();
}

bool VinoEngine::InferenceFloat32(
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
        // Set input data
        if (!input_names_.empty()) {
            auto input_tensor = ov::Tensor(ov::element::f32, input_shapes_[0], const_cast<float*>(input_data));
            infer_request_->set_input_tensor(0, input_tensor);
        }

        // Run inference
        infer_request_->infer();

        // Get output
        if (!output_names_.empty()) {
            auto output_tensor = infer_request_->get_output_tensor(0);
            float* out_data = output_tensor.data<float>();
            std::copy(out_data, out_data + output_count, output_data);
        }

        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Inference failed: ") + e.what();
        if (error_msg) *error_msg = last_error_;
        return false;
    } 
}

bool VinoEngine::SetDevice(const std::string& device_name) { 
    try {
        device_name_ = device_name;
        if (model_) {
            compiled_model_ = core_->compile_model(model_, device_name);
            infer_request_ = compiled_model_->create_infer_request();
        }
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Device switch failed: ") + e.what();
        return false;
    } 
}

void VinoEngine::CacheModelInfo() { 
    if (!model_) return;

    auto inputs = model_->inputs();
    for (const auto& input : inputs) {
        input_names_.push_back(input.get_node()->get_friendly_name());
        auto shape = input.get_shape();
        input_shapes_.push_back(std::vector<int64_t>(shape.begin(), shape.end()));
    }

    auto outputs = model_->outputs();
    for (const auto& output : outputs) {
        output_names_.push_back(output.get_node()->get_friendly_name());
        auto shape = output.get_shape();
        output_shapes_.push_back(std::vector<int64_t>(shape.begin(), shape.end()));
    } 
}

std::string VinoEngine::GetLastError() const {
    return last_error_;
}

}  // namespace AsyncIO::IO::DNN
