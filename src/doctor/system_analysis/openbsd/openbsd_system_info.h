#pragma once
#include "system_analysis/base/common_system_info.h"

namespace EngineDoctor {

class OpenBsdSystemInfo : public CommonSystemInfoReader {
public:
    CommonSystemInfo Read() override;
};

} // namespace EngineDoctor
