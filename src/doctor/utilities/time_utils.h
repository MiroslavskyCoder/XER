#pragma once
#include <cstdint>
#include <string>

namespace EngineDoctor {
namespace TimeUtils {

uint64_t NowMs();
std::string FormatTimestamp(uint64_t ms);
void SleepMs(int ms);

}  // namespace TimeUtils
}  // namespace EngineDoctor
