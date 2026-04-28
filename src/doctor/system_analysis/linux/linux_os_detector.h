/**
 * @file linux_os_detector.h
 * @brief Детектор операционной системы Linux.
 * @author Yoshi Arasaka (main)
 * @date 2023-10-27
 * @copyright MIT License
 */

#pragma once

#include "system_analysis/os_detector_interface.h"
#include "core/engine_doctor_context.h"

namespace EngineDoctor {

class LinuxOSDetector : public OSDetectorInterface {
public:
    LinuxOSDetector(Context& context);
    ~LinuxOSDetector() override = default;

    bool is_os() const override;
    std::string get_os_name() const override;

private:
    // Методы для проверки специфичных признаков Linux
    bool check_proc_files() const;
    bool check_sys_files() const;
    // ...
};

} // namespace EngineDoctor
