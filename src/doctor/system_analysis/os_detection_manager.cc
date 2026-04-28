/**
 * @file os_detection_manager.cc
 * @brief Реализация менеджера определения операционной системы.
 * @author Yoshi Arasaka (main)
 * @date 2023-10-27
 * @copyright MIT License
 */

#include "system_analysis/os_detection_manager.h"
#include "core/logger.h"
#include "system_analysis/linux/linux_os_detector.h"
#include "system_analysis/windows/windows_os_detector.h"
#include "system_analysis/macos/macos_os_detector.h"
#include "system_analysis/openbsd/openbsd_os_detector.h"

namespace EngineDoctor {

OSDetectionManager::OSDetectionManager(Context& context) : context_(context) {
    initialize_detectors();
}

void OSDetectionManager::initialize_detectors() {
    Logger::debug("OSDetectionManager: Initializing OS detectors.");
    // Регистрация детекторов ОС
    os_detectors_.push_back(std::make_unique<LinuxOSDetector>(context_));
    os_detectors_.push_back(std::make_unique<WindowsOSDetector>(context_));
    os_detectors_.push_back(std::make_unique<MacOSOSDetector>(context_));
    os_detectors_.push_back(std::make_unique<OpenBSDOSDetector>(context_));
    // ... добавьте другие детекторов при необходимости
    Logger::debug("OSDetectionManager: OS detectors initialized.");
}

std::string OSDetectionManager::detect_os() {
    if (!detected_os_.empty()) {
        return detected_os_;
    }

    Logger::debug("OSDetectionManager: Attempting to detect OS...");
    for (const auto& detector : os_detectors_) {
        if (detector->is_os()) {
            detected_os_ = detector->get_os_name();
            Logger::info("OSDetectionManager: Detected OS: %s", detected_os_.c_str());
            return detected_os_;
        }
    }

    Logger::warning("OSDetectionManager: Could not detect OS. Returning 'unknown'.");
    detected_os_ = "unknown";
    return detected_os_;
}

} // namespace EngineDoctor
