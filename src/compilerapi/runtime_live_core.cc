#include "runtime_live.h"

#include <utility>

RuntimeLive::RuntimeLive() : running_(false) {}

bool RuntimeLive::is_running() const {
    return running_;
}

void RuntimeLive::set_running(bool running) {
    running_ = running;
}

RuntimeLive::InterfaceCompiler RuntimeLive::CreateInterfaceCompiler(
    const InterfaceCompilerOptions& options) {
    return InterfaceCompiler(options.cache_dir,
                             options.provider,
                             options.language,
                             options.compile_flags,
                             options.link_flags);
}
