
#pragma once

#include <filesystem>
#include <string>
#include <vector>

class CompilerSource {
public:
    CompilerSource(std::string source,
                   std::string cache_dir,
                   std::string provider,
                   std::string language,
                   std::string compile_flags,
                   std::string link_flags,
                   std::vector<std::string> source_files = {});

    const std::string& source() const;

    std::string source_with_stdio() const;

    bool use_cpp() const;

    const std::filesystem::path& cache_dir() const;

    std::filesystem::path source_path() const;

    std::filesystem::path binary_path() const;

    std::filesystem::path compile_out_path() const;

    std::filesystem::path compile_err_path() const;

    std::filesystem::path run_out_path() const;

    std::filesystem::path run_err_path() const;

    std::vector<std::string> BuildCompilerArgs() const;

    std::string BuildCompilerCommandPreview(const std::string& compiler_binary) const;

private:
    std::vector<std::string> SplitFlags(const std::string& flags) const;

    std::string QuoteForShell(const std::string& token) const;

    std::string BuildProviderDefine() const;

    std::string source_;
    std::filesystem::path cache_dir_;
    std::string provider_;
    std::string language_;
    std::string compile_flags_;
    std::string link_flags_;
    std::vector<std::string> source_files_;
};