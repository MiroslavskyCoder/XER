#include "fix_state_rollback.h"

namespace AutoCorrection {

void FixStateRollback::Push(const std::string& state) {
    undo_stack_.push(state);
}

std::string FixStateRollback::Pop() {
    if (undo_stack_.empty()) return {};
    std::string top = undo_stack_.top();
    undo_stack_.pop();
    return top;
}

bool FixStateRollback::CanUndo() const {
    return !undo_stack_.empty();
}

void FixStateRollback::PushRedo(const std::string& state) {
    redo_stack_.push(state);
}

std::string FixStateRollback::PopRedo() {
    if (redo_stack_.empty()) return {};
    std::string top = redo_stack_.top();
    redo_stack_.pop();
    return top;
}

bool FixStateRollback::CanRedo() const {
    return !redo_stack_.empty();
}

} // namespace AutoCorrection
