#include "xer/xer_buffer.h"

#include <fstream>

namespace Xer {

XerBuffer::XerBuffer(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}

XerBuffer XerBuffer::FromString(std::string_view text) {
    XerBuffer buffer;
    buffer.Append(text);
    return buffer;
}

XerBuffer XerBuffer::FromFile(const std::filesystem::path& path, std::string* error_out) {
    if (!std::filesystem::exists(path)) {
        if (error_out != nullptr) {
            *error_out = "buffer source file does not exist";
        }
        return XerBuffer();
    }

    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        if (error_out != nullptr) {
            *error_out = "failed to open buffer source file";
        }
        return XerBuffer();
    }

    input.seekg(0, std::ios::end);
    const std::streamsize size = input.tellg();
    input.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> bytes;
    if (size > 0) {
        bytes.resize(static_cast<std::size_t>(size));
        input.read(reinterpret_cast<char*>(bytes.data()), size);
    }

    if (!input && size > 0) {
        if (error_out != nullptr) {
            *error_out = "failed to read buffer source file";
        }
        return XerBuffer();
    }

    if (error_out != nullptr) {
        error_out->clear();
    }
    return XerBuffer(std::move(bytes));
}

void XerBuffer::Append(std::string_view text) {
    bytes_.insert(bytes_.end(), text.begin(), text.end());
}

void XerBuffer::Append(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) {
        return;
    }
    bytes_.insert(bytes_.end(), data, data + size);
}

std::string XerBuffer::ToString() const {
    return std::string(bytes_.begin(), bytes_.end());
}

}  // namespace Xer