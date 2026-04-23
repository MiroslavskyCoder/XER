#include "async_file_reader.h"

#include "io_thread_pool.h"

#include <fstream>

namespace IO::AsyncIO {

AsyncFileReader::AsyncFileReader()
    : file_handle_(nullptr), file_size_(0), current_position_(0), is_reading_(false), pending_ops_(0) {}

AsyncFileReader::~AsyncFileReader() {
    CloseFile();
}

bool AsyncFileReader::OpenFile(const std::string& filepath) {
    try {
        std::scoped_lock lock(state_mutex_);
        if (file_handle_ != nullptr) {
            std::ifstream* existing_file = static_cast<std::ifstream*>(file_handle_);
            existing_file->close();
            delete existing_file;
            file_handle_ = nullptr;
            file_size_ = 0;
            current_position_ = 0;
        }

        // Use standard ifstream for file access
        std::ifstream* file = new std::ifstream(filepath, std::ios::binary);
        if (!file->is_open()) {
            last_error_ = "Failed to open file: " + filepath;
            delete file;
            return false;
        }

        file_handle_ = file;
        if (!QueryFileSize()) {
            return false;
        }

        current_position_ = 0;
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Open exception: ") + e.what();
        return false;
    }
}

bool AsyncFileReader::IsFileOpen() const {
    std::scoped_lock lock(state_mutex_);
    return file_handle_ != nullptr;
}

void AsyncFileReader::CloseFile() {
    std::scoped_lock lock(state_mutex_);
    if (file_handle_) {
        std::ifstream* file = static_cast<std::ifstream*>(file_handle_);
        file->close();
        delete file;
        file_handle_ = nullptr;
        file_size_ = 0;
        current_position_ = 0;
    }
}

bool AsyncFileReader::ReadAsync(size_t bytes_to_read, ReadCallback callback) {
    std::ifstream* file = nullptr;
    {
        std::scoped_lock lock(state_mutex_);
        if (!file_handle_) {
            last_error_ = "File not open";
            return false;
        }

        if (is_reading_) {
            last_error_ = "Read already in progress";
            return false;
        }

        is_reading_ = true;
        ++pending_ops_;
        file = static_cast<std::ifstream*>(file_handle_);
    }

    IOThreadPool::GetSharedInstance().Enqueue([this, bytes_to_read, callback, file]() {
        std::vector<uint8_t> buffer(bytes_to_read);

        size_t bytes_read = 0;
        int error_code = 0;
        {
            std::scoped_lock lock(state_mutex_);
            if (file_handle_ != file) {
                error_code = 1;
            } else {
                file->read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(bytes_to_read));
                bytes_read = static_cast<size_t>(file->gcount());
                current_position_ += static_cast<int64_t>(bytes_read);
                error_code = (file->bad() ? 1 : 0);
            }
        }

        if (callback) {
            callback(error_code, bytes_read, buffer.data());
        }

        std::scoped_lock lock(state_mutex_);
        is_reading_ = false;
        --pending_ops_;
    });
    return true;
}

bool AsyncFileReader::ReadSync(size_t bytes_to_read, std::vector<uint8_t>& output_buffer) {
    try {
        std::scoped_lock lock(state_mutex_);
        if (!file_handle_) {
            last_error_ = "File not open";
            return false;
        }

        std::ifstream* file = static_cast<std::ifstream*>(file_handle_);
        output_buffer.resize(bytes_to_read);

        file->read(reinterpret_cast<char*>(output_buffer.data()), static_cast<std::streamsize>(bytes_to_read));
        size_t bytes_read = static_cast<size_t>(file->gcount());
        current_position_ += static_cast<int64_t>(bytes_read);

        output_buffer.resize(bytes_read);
        return !file->bad();
    } catch (const std::exception& e) {
        last_error_ = std::string("Read exception: ") + e.what();
        return false;
    }
}

bool AsyncFileReader::Seek(int64_t offset) {
    try {
        std::scoped_lock lock(state_mutex_);
        if (!file_handle_) {
            last_error_ = "File not open";
            return false;
        }

        std::ifstream* file = static_cast<std::ifstream*>(file_handle_);
        file->seekg(offset, std::ios::beg);
        current_position_ = offset;
        return !file->fail();
    } catch (...) {
        last_error_ = "Seek failed";
        return false;
    }
}

int64_t AsyncFileReader::GetPosition() const {
    std::scoped_lock lock(state_mutex_);
    return current_position_;
}

int64_t AsyncFileReader::GetFileSize() const {
    std::scoped_lock lock(state_mutex_);
    return file_size_;
}

bool AsyncFileReader::IsReading() const {
    std::scoped_lock lock(state_mutex_);
    return is_reading_;
}

int AsyncFileReader::GetPendingOperations() const {
    std::scoped_lock lock(state_mutex_);
    return pending_ops_;
}

bool AsyncFileReader::QueryFileSize() {
    try {
        std::ifstream* file = static_cast<std::ifstream*>(file_handle_);
        file->seekg(0, std::ios::end);
        file_size_ = file->tellg();
        file->seekg(0, std::ios::beg);
        return true;
    } catch (...) {
        last_error_ = "Failed to query file size";
        return false;
    }
}

std::string AsyncFileReader::GetLastError() const {
    std::scoped_lock lock(state_mutex_);
    return last_error_;
}

}  // namespace AIToolsXPro::IO::AsyncIO
