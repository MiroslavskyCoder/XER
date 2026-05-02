#include "fix_version_control.h"

#include <array>
#include <stdexcept>
#include <cstdio>
#include <sstream>

namespace AutoCorrection {

std::string FixVersionControl::RunGit(const std::string& args) {
    // Build the command; args must be validated by callers before reaching here.
    std::string cmd = "git " + args + " 2>&1";
    std::array<char, 512> buffer{};
    std::string output;

    std::FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};

    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
        output += buffer.data();
    }
    pclose(pipe);
    return output;
}

bool FixVersionControl::Commit(const std::string& msg) {
    // Reject shell-special characters to prevent injection.
    for (char c : msg) {
        if (c == '\'' || c == '"' || c == '\\' || c == '`' || c == '$' || c == '\n') {
            return false;
        }
    }
    std::string out = RunGit("commit -m '" + msg + "'");
    return out.find("master") != std::string::npos ||
           out.find("main")   != std::string::npos ||
           out.find("branch") != std::string::npos;
}

bool FixVersionControl::Revert(const std::string& hash) {
    // Accept only hex characters to avoid shell injection.
    for (char c : hash) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    std::string out = RunGit("revert --no-edit " + hash);
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

std::vector<std::string> FixVersionControl::GetLog() {
    std::string out = RunGit("log --oneline -20");
    std::vector<std::string> lines;
    std::istringstream stream(out);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

} // namespace AutoCorrection
