#pragma once

#include <string>
#include <vector>

namespace AutoCorrection {

class FixVersionControl {
public:
    bool Commit(const std::string& msg);
    bool Revert(const std::string& hash);
    std::vector<std::string> GetLog();

private:
    std::string RunGit(const std::string& args);
};

} // namespace AutoCorrection
