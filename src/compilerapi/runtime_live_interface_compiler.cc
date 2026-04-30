#include "runtime_live.h"
#include <utility>

RuntimeLive::InterfaceCompiler::InterfaceCompiler(std::string cache_dir,
                                                  std::string provider,
                                                  std::string language,
                                                  std::string compile_flags,
                                                  std::string link_flags)
    : cache_dir_(std::move(cache_dir)),
      provider_(std::move(provider)),
      language_(std::move(language)),
      compile_flags_(std::move(compile_flags)),
      link_flags_(std::move(link_flags)) {}

void RuntimeLive::InterfaceCompiler::AddEntryRaw(std::string source) {
    source_ = std::move(source);
}

void RuntimeLive::InterfaceCompiler::AddEntryFile(const SourceFile& source_file) {
    source_files_.push_back(source_file);
}

void RuntimeLive::InterfaceCompiler::ClearEntryFiles() {
    source_files_.clear();
}
