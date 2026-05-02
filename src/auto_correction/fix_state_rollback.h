#pragma once

#include <string>
#include <stack>

namespace AutoCorrection {

class FixStateRollback {
public:
    void Push(const std::string& state);
    std::string Pop();
    bool CanUndo() const;

    void PushRedo(const std::string& state);
    std::string PopRedo();
    bool CanRedo() const;

private:
    std::stack<std::string> undo_stack_;
    std::stack<std::string> redo_stack_;
};

} // namespace AutoCorrection
