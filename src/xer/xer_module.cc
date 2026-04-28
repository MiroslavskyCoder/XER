#include "xer/xer_module.h"

#include <range/v3/view/transform.hpp>

#include <algorithm>

namespace Xer {

XerModuleRegistry::XerModuleRegistry() {
    perf_counter_.Enable();
}

void XerModuleRegistry::Register(std::string name, XerModuleHandler handler) {
    handlers_[std::move(name)] = std::move(handler);
}

bool XerModuleRegistry::Has(const std::string& name) const {
    return handlers_.contains(name);
}

XerValue XerModuleRegistry::Invoke(const std::string& name, const XerModuleCallContext& context, std::string* error_out) const {
    const auto it = handlers_.find(name);
    if (it == handlers_.end()) {
        if (error_out != nullptr) {
            *error_out = "module is not registered";
        }
        return XerValue();
    }

    perf_counter_.StartCounter("xer_module_invoke");
    XerValue value = it->second(context, error_out);
    perf_counter_.StopCounter("xer_module_invoke");
    return value;
}

std::vector<std::string> XerModuleRegistry::Names() const {
    std::vector<std::string> names;
    const auto name_view = handlers_ | ranges::views::transform([](const auto& entry) { return entry.first; });
    for (const auto& name : name_view) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::string XerModuleRegistry::Report() const {
    return perf_counter_.GetReport();
}

}  // namespace Xer