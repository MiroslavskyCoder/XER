#pragma once

#include <filesystem>
#include <string>
#include <vector>

class CompilerApi {
public:
    bool Compile(const std::string& compiler_binary,
                 const std::vector<std::string>& args,
                 const std::filesystem::path& stdout_path,
                 const std::filesystem::path& stderr_path,
                 int* exit_code) const;

    bool RunBinary(const std::filesystem::path& binary_path,
                   const std::filesystem::path& stdout_path,
                   const std::filesystem::path& stderr_path,
                   int* exit_code) const;

private:
    bool CompileInProcess(const std::string& compiler_binary,
                          const std::vector<std::string>& args,
                          const std::filesystem::path& stdout_path,
                          const std::filesystem::path& stderr_path,
                          int* exit_code) const;

    bool ExecuteProcess(const std::string& program,
                        const std::vector<std::string>& args,
                        const std::filesystem::path& stdout_path,
                        const std::filesystem::path& stderr_path,
                        int* exit_code,
                        bool resolve_program) const;
};
