/**
 * @file os_detection_manager.h
 * @brief Управляет определением операционной системы.
 * @author Yoshi Arasaka (main)
 * @date 2023-10-27
 * @copyright MIT License
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "core/engine_doctor_context.h"
#include "system_analysis/os_detector_interface.h"

namespace EngineDoctor {

class OSDetectionManager {
public:
    OSDetectionManager(Context& context);
    ~OSDetectionManager() = default;

    /**
     * @brief Определяет текущую операционную систему.
     * @return Имя ОС (например, "linux", "windows", "macos").
     */
    std::string detect_os();

private:
    Context& context_;
    std::string detected_os_;

    // Список доступных детекторов ОС
    std::vector<std::unique_ptr<OSDetectorInterface>> os_detectors_;

    void initialize_detectors();
};

} // namespace EngineDoctor
