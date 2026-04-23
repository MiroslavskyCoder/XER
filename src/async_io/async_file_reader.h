#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <mutex>

namespace IO::AsyncIO {

// Read completion callback: (error_code, bytes_read, data)
using ReadCallback = std::function<void(int, size_t, const uint8_t*)>;

class AsyncFileReader {
public:
    AsyncFileReader();
    ~AsyncFileReader();

    // File operations
    bool OpenFile(const std::string& filepath);
    bool IsFileOpen() const;
    void CloseFile();

    // Async read
    bool ReadAsync(size_t bytes_to_read, ReadCallback callback);
    
    // Sync read (blocking)
    bool ReadSync(size_t bytes_to_read, std::vector<uint8_t>& output_buffer);

    // Stream control
    bool Seek(int64_t offset);
    int64_t GetPosition() const;
    int64_t GetFileSize() const;

    // Status
    bool IsReading() const;
    int GetPendingOperations() const;

    // Error tracking
    std::string GetLastError() const;

private:
    mutable std::mutex state_mutex_;
    void* file_handle_;
    int64_t file_size_;
    int64_t current_position_;
    bool is_reading_;
    int pending_ops_;
    std::string last_error_;

    bool QueryFileSize();
};

}  // namespace AIToolsXPro::IO::AsyncIO
