#include "xer/xer_value.h"

#include <sstream>

namespace Xer {

namespace {

const std::string& EmptyString() {
    static const std::string empty;
    return empty;
}

}  // namespace

XerValue::XerValue() = default;

XerValue::XerValue(std::int64_t value) : data_(value) {}

XerValue::XerValue(std::string value) : data_(std::move(value)) {}

XerValue::XerValue(const char* value) : data_(std::string(value == nullptr ? "" : value)) {}

XerValue::XerValue(std::shared_ptr<Object> object) : data_(std::move(object)) {}

XerValue::XerValue(std::shared_ptr<Array> array) : data_(std::move(array)) {}

XerValue XerValue::MakeObject() {
    return XerValue(std::make_shared<Object>());
}

XerValue XerValue::MakeArray() {
    return XerValue(std::make_shared<Array>());
}

XerValueType XerValue::type() const {
    if (std::holds_alternative<std::monostate>(data_)) {
        return XerValueType::kNull;
    }
    if (std::holds_alternative<std::int64_t>(data_)) {
        return XerValueType::kInteger;
    }
    if (std::holds_alternative<std::string>(data_)) {
        return XerValueType::kString;
    }
    if (std::holds_alternative<std::shared_ptr<Object>>(data_)) {
        return XerValueType::kObject;
    }
    return XerValueType::kArray;
}

bool XerValue::IsNull() const {
    return type() == XerValueType::kNull;
}

bool XerValue::IsInteger() const {
    return type() == XerValueType::kInteger;
}

bool XerValue::IsString() const {
    return type() == XerValueType::kString;
}

bool XerValue::IsObject() const {
    return type() == XerValueType::kObject;
}

bool XerValue::IsArray() const {
    return type() == XerValueType::kArray;
}

std::int64_t XerValue::AsInteger(std::int64_t fallback) const {
    if (const auto* integer = std::get_if<std::int64_t>(&data_)) {
        return *integer;
    }
    return fallback;
}

const std::string& XerValue::AsString() const {
    if (const auto* text = std::get_if<std::string>(&data_)) {
        return *text;
    }
    return EmptyString();
}

std::shared_ptr<XerValue::Object> XerValue::AsObject() const {
    if (const auto* object = std::get_if<std::shared_ptr<Object>>(&data_)) {
        return *object;
    }
    return {};
}

std::shared_ptr<XerValue::Array> XerValue::AsArray() const {
    if (const auto* array = std::get_if<std::shared_ptr<Array>>(&data_)) {
        return *array;
    }
    return {};
}

std::string XerValue::ToString() const {
    if (const auto* integer = std::get_if<std::int64_t>(&data_)) {
        return std::to_string(*integer);
    }
    if (const auto* text = std::get_if<std::string>(&data_)) {
        return *text;
    }
    if (const auto* object = std::get_if<std::shared_ptr<Object>>(&data_)) {
        std::ostringstream stream;
        stream << "{";
        bool first = true;
        if (*object) {
            for (const auto& [key, value] : **object) {
                if (!first) {
                    stream << ", ";
                }
                first = false;
                stream << key << ": " << value.ToString();
            }
        }
        stream << "}";
        return stream.str();
    }
    if (const auto* array = std::get_if<std::shared_ptr<Array>>(&data_)) {
        std::ostringstream stream;
        stream << "[";
        bool first = true;
        if (*array) {
            for (const auto& value : **array) {
                if (!first) {
                    stream << ", ";
                }
                first = false;
                stream << value.ToString();
            }
        }
        stream << "]";
        return stream.str();
    }
    return "null";
}

}  // namespace Xer