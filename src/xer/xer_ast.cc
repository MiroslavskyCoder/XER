#include "xer/xer_ast.h"

namespace Xer {

std::string XerAstNodeKindName(XerAstNodeKind kind) {
    switch (kind) {
        case XerAstNodeKind::kProgram:
            return "program";
        case XerAstNodeKind::kImport:
            return "import";
        case XerAstNodeKind::kFunction:
            return "function";
        case XerAstNodeKind::kLocal:
            return "local";
        case XerAstNodeKind::kCall:
            return "call";
        case XerAstNodeKind::kLiteral:
            return "literal";
    }
    return "unknown";
}

}  // namespace Xer