#include "xer/xer_util.h"

#include <absl/strings/ascii.h>
#include <absl/strings/str_format.h>

namespace Xer {

std::string XerUtil::ReadSourceFile(const std::filesystem::path& path, std::string* error_out) {
    if (!std::filesystem::exists(path)) {
        if (error_out != nullptr) {
            *error_out = "source file does not exist";
        }
        return std::string();
    }

    if (error_out != nullptr) {
        error_out->clear();
    }
    return ToolTo::ReadTextFile(path);
}

std::string XerUtil::ExtensionOf(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    absl::AsciiStrToLower(&extension);
    return extension;
}

bool XerUtil::IsXerPath(const std::filesystem::path& path) {
    return ExtensionOf(path) == ".xer";
}

std::string XerUtil::FormatLocation(const XerSourceLocation& location) {
    return absl::StrFormat("%zu:%zu", location.line, location.column);
}

}  // namespace Xer