#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace AsyncIO::IO::DNN {

class OnnxRuntimeEngine;

class OnnxSessionManager {
public:
    OnnxSessionManager();
    ~OnnxSessionManager();

    // Session lifecycle
    bool LoadModel(const std::string& session_id, const std::string& model_path);
    bool UnloadModel(const std::string& session_id);
    bool HasSession(const std::string& session_id) const;

    // Model inference through session
    bool RunInference(
        const std::string& session_id,
        const float* input_data,
        size_t input_count,
        float* output_data,
        size_t output_count,
        std::string* error_msg = nullptr
    );

    // Batch operations
    bool BatchInference(
        const std::string& session_id,
        const std::vector<const float*>& batch_inputs,
        const std::vector<size_t>& input_counts,
        std::vector<float*>& batch_outputs,
        const std::vector<size_t>& output_counts
    );

    // Session management
    std::vector<std::string> GetLoadedSessions() const;
    void ClearAllSessions();

    // Error tracking
    std::string GetLastError() const;

private:
    mutable std::mutex sessions_mutex_;
    std::map<std::string, std::unique_ptr<OnnxRuntimeEngine>> sessions_;
    std::string last_error_;
};

}  // namespace AsyncIO::IO::DNN
