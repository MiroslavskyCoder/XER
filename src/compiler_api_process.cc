#include "compiler_api.h"

#include "tool_to.h"

#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/Program.h>

#include <array>
#include <cstdlib>
#include <optional>
#include <sys/wait.h>

namespace {

std::string QuoteForShell(const std::string& token) {
    std::string out = "'";
    for (char ch : token) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out.push_back(ch);
        }
    }
    out.push_back('\'');
    return out;
}

}  // namespace

bool CompilerApi::ExecuteProcess(
    const std::string& program,
    const std::vector<std::string>& args,
    const std::filesystem::path& stdout_path,
    const std::filesystem::path& stderr_path,
    int* exit_code,
    bool resolve_program) const {
    {
        std::string io_error;
        const bool out_ok = ToolTo::WriteTextFile(stdout_path, std::string(), false, &io_error);
        const bool err_ok = ToolTo::WriteTextFile(stderr_path, std::string(), false, &io_error);
        if (!out_ok || !err_ok) {
            return false;
        }
    }

    std::string executable = program;
    if (resolve_program) {
        llvm::ErrorOr<std::string> resolved_program = llvm::sys::findProgramByName(program);
        if (!resolved_program) {
            return false;
        }
        executable = *resolved_program;
    } else {
        std::error_code ec;
        const std::filesystem::path abs_program = std::filesystem::absolute(program, ec);
        if (!ec) {
            executable = abs_program.string();
        }
    }

    int local_exit_code = -1;
    if (!resolve_program) {
        std::string command = QuoteForShell(executable);
        for (const std::string& arg : args) {
            command += " " + QuoteForShell(arg);
        }
        command += " > " + QuoteForShell(std::filesystem::absolute(stdout_path).string());
        command += " 2> " + QuoteForShell(std::filesystem::absolute(stderr_path).string());

        const int system_rc = std::system(command.c_str());
        if (system_rc == -1) {
            return false;
        }

        if (WIFEXITED(system_rc)) {
            local_exit_code = WEXITSTATUS(system_rc);
        } else {
            local_exit_code = -1;
        }
    } else {
        std::vector<llvm::StringRef> argv;
        argv.reserve(args.size() + 1);
        argv.push_back(executable);
        for (const std::string& arg : args) {
            argv.push_back(arg);
        }

        std::array<std::optional<llvm::StringRef>, 3> redirects;
        redirects[0] = std::nullopt;
        redirects[1] = std::filesystem::absolute(stdout_path).string();
        redirects[2] = std::filesystem::absolute(stderr_path).string();

        local_exit_code = llvm::sys::ExecuteAndWait(executable, argv, std::nullopt, redirects);
    }

    if (exit_code != nullptr) {
        *exit_code = local_exit_code;
    }
    return true;
}
