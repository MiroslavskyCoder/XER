#include "xer/xer_local.h"

namespace Xer {

void XerLocal::Push() {
    frames_.emplace_back();
}

void XerLocal::Pop() {
    if (!frames_.empty()) {
        frames_.pop_back();
    }
}

void XerLocal::Set(std::string name, XerValue value) {
    if (frames_.empty()) {
        Push();
    }
    frames_.back()[std::move(name)] = std::move(value);
}

const XerValue* XerLocal::Get(const std::string& name) const {
    for (auto it = frames_.rbegin(); it != frames_.rend(); ++it) {
        const auto value_it = it->find(name);
        if (value_it != it->end()) {
            return &value_it->second;
        }
    }
    return nullptr;
}

bool XerLocal::Contains(const std::string& name) const {
    return Get(name) != nullptr;
}

}  // namespace Xer