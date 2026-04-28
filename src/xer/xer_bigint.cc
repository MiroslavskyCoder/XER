#include "xer/xer_bigint.h"

#include <algorithm>
#include <limits>

namespace Xer {

XerBigInt::XerBigInt() : decimal_("0") {}

XerBigInt::XerBigInt(std::string decimal) : decimal_(std::move(decimal)) {}

XerBigInt XerBigInt::FromString(const std::string& text, std::string* error_out) {
    if (text.empty()) {
        if (error_out != nullptr) {
            *error_out = "bigint text is empty";
        }
        return XerBigInt();
    }

    std::size_t index = 0;
    if (text[0] == '+' || text[0] == '-') {
        index = 1;
    }
    if (index == text.size()) {
        if (error_out != nullptr) {
            *error_out = "bigint text has no digits";
        }
        return XerBigInt();
    }

    for (; index < text.size(); ++index) {
        if (text[index] < '0' || text[index] > '9') {
            if (error_out != nullptr) {
                *error_out = "bigint contains non-digit characters";
            }
            return XerBigInt();
        }
    }

    if (error_out != nullptr) {
        error_out->clear();
    }
    return XerBigInt(text);
}

bool XerBigInt::FitsInt64() const {
    std::string digits = decimal_;
    bool negative = false;
    if (!digits.empty() && (digits[0] == '+' || digits[0] == '-')) {
        negative = digits[0] == '-';
        digits.erase(digits.begin());
    }

    digits.erase(0, std::min(digits.find_first_not_of('0'), digits.size() - 1));
    const std::string limit = negative ? "9223372036854775808" : "9223372036854775807";
    if (digits.size() != limit.size()) {
        return digits.size() < limit.size();
    }
    return digits <= limit;
}

std::int64_t XerBigInt::ToInt64(std::int64_t fallback) const {
    if (!FitsInt64()) {
        return fallback;
    }
    try {
        return std::stoll(decimal_);
    } catch (...) {
        return fallback;
    }
}

}  // namespace Xer