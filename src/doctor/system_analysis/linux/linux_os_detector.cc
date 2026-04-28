/**
 * @file linux_os_detector.cc
 * @brief Реализация детектора операционной системы Linux.
 * @author Yoshi Arasaka (main)
 * @date 2023-10-27
 * @copyright MIT License
 */

#include "system_analysis/linux/linux_os_detector.h"
#include "core/logger.h"
#include <fstream>
#include <string>
#include <cstdio>

namespace EngineDoctor {

LinuxOSDetector::LinuxOSDetector(Context& context) : OSDetectorInterface(context) {}

bool LinuxOSDetector::is_os() const {
    Logger::debug("LinuxOSDetector: Checking for Linux OS...");
    // Типичный способ определения Linux: проверка наличия /proc/version или /etc/os-release
    return check_proc_files() || check_sys_files();
}

std::string LinuxOSDetector::get_os_name() const {
    return "linux";
}

bool LinuxOSDetector::check_proc_files() const {
    std::ifstream version_file("/proc/version");
    return version_file.is_open();
}

bool LinuxOSDetector::check_sys_files() const {
    // Дополнительные проверки, например, наличие /sys/kernel/osrelease
    // Или чтение /etc/os-release
    std::ifstream os_release_file("/etc/os-release");
    if (os_release_file.is_open()) {
        std::string line;
        while (std::getline(os_release_file, line)) {
            if (line.find("ID=") != std::string::npos) {
                // Можно добавить более точную проверку ID
                return true;
            }
        }
    }
    return false;
}

} // namespace EngineDoctor
