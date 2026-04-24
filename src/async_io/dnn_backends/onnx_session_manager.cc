#include "onnx_session_manager.h"
#include "onnx_runtime_engine.h"

namespace AsyncIO::IO::DNN {

OnnxSessionManager::OnnxSessionManager() {}

OnnxSessionManager::~OnnxSessionManager() {
    ClearAllSessions();
}

bool OnnxSessionManager::LoadModel(const std::string& session_id, const std::string& model_path) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto engine = std::make_unique<OnnxRuntimeEngine>();
    if (!engine->Initialize(model_path)) {
        last_error_ = engine->GetLastError();
        return false;
    }

    sessions_[session_id] = std::move(engine);
    return true;
}

bool OnnxSessionManager::UnloadModel(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        sessions_.erase(it);
        return true;
    }
    
    last_error_ = "Session not found: " + session_id;
    return false;
}

bool OnnxSessionManager::HasSession(const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.find(session_id) != sessions_.end();
}

bool OnnxSessionManager::RunInference(
    const std::string& session_id,
    const float* input_data,
    size_t input_count,
    float* output_data,
    size_t output_count,
    std::string* error_msg) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        last_error_ = "Session not found: " + session_id;
        if (error_msg) *error_msg = last_error_;
        return false;
    }

    return it->second->InferenceFloat32(input_data, input_count, output_data, output_count, error_msg);
}

bool OnnxSessionManager::BatchInference(
    const std::string& session_id,
    const std::vector<const float*>& batch_inputs,
    const std::vector<size_t>& input_counts,
    std::vector<float*>& batch_outputs,
    const std::vector<size_t>& output_counts) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        last_error_ = "Session not found: " + session_id;
        return false;
    }

    auto& engine = it->second;
    for (size_t i = 0; i < batch_inputs.size(); ++i) {
        if (!engine->InferenceFloat32(
                batch_inputs[i],
                input_counts[i],
                batch_outputs[i],
                output_counts[i])) {
            last_error_ = engine->GetLastError();
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> OnnxSessionManager::GetLoadedSessions() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    std::vector<std::string> sessions;
    for (const auto& pair : sessions_) {
        sessions.push_back(pair.first);
    }
    return sessions;
}

void OnnxSessionManager::ClearAllSessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_.clear();
}

std::string OnnxSessionManager::GetLastError() const {
    return last_error_;
}

}  // namespace AsyncIO::IO::DNN
