#include "xer/xer_contrustor.h"

namespace Xer {

XerAstImport XerContrustor::MakeImport(std::string module_name, std::string alias, XerSourceRange range) const {
    return XerAstImport{std::move(module_name), std::move(alias), range};
}

XerAstFunction XerContrustor::MakeFunction(std::string name, bool is_async, XerSourceRange range) const {
    XerAstFunction function;
    function.name = std::move(name);
    function.is_async = is_async;
    function.range = range;
    return function;
}

XerAstCall XerContrustor::MakeCall(std::string callee, std::optional<std::string> receiver, XerSourceRange range) const {
    XerAstCall call;
    call.callee = std::move(callee);
    call.receiver = std::move(receiver);
    call.range = range;
    return call;
}

void XerContrustor::AddLocal(XerAstFunction* function, std::string name) const {
    if (function != nullptr) {
        function->locals.push_back(std::move(name));
    }
}

void XerContrustor::AddCall(XerAstFunction* function, XerAstCall call) const {
    if (function != nullptr) {
        function->calls.push_back(std::move(call));
    }
}

void XerContrustor::AddTopLevelCall(XerAstProgram* program, XerAstCall call) const {
    if (program != nullptr) {
        program->top_level_calls.push_back(std::move(call));
    }
}

}  // namespace Xer