#pragma once

namespace Engine::V8Runtime {

void EnsureInitialized(int platform_workers);
void Shutdown();

}  // namespace Engine::V8Runtime
