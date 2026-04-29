/**
 * @file engine_doctor_core.cc
 * @brief Реализация основного класса EngineDoctor.
 * @author byMiroslavsky (main)
 * @author LXXV (contributor)
 * @date 2023-10-27
 * @copyright MIT License
 */

#include "core/engine_doctor_core.h"
#include "core/logger.h"
#include "core/exceptions.h"
#include "scanner/scan_parameters.h"
#include "report/report_generator_interface.h"

namespace EngineDoctor {

EngineDoctorCore::EngineDoctorCore(const Config& config)
        : config_(config),
            context_(),
            module_manager_(context_) {
    // Инициализация логгера с настройками из конфигурации
    Logger::initialize(config_.log_level);
    Logger::info("EngineDoctorCore: Initializing...");

    // Создание и регистрация модулей через ModuleManager
    module_manager_.register_module(std::make_unique<ScannerModule>(context_));
    module_manager_.register_module(std::make_unique<AnalysisEngine>(context_));
    // ... регистрация других модулей

    Logger::info("EngineDoctorCore: Initialization complete.");
}

EngineDoctorCore::~EngineDoctorCore() {
    Logger::info("EngineDoctorCore: Shutting down...");
    // Очистка ресурсов, освобождение памяти
    Logger::info("EngineDoctorCore: Shutdown complete.");
}

void EngineDoctorCore::run(const ScanParameters& params) {
    Logger::info("EngineDoctorCore: Starting run with parameters.");
    initialize_modules();
    perform_scan(params);
    perform_analysis();
    generate_report();
    Logger::info("EngineDoctorCore: Run finished.");
}

void EngineDoctorCore::initialize_modules() {
    Logger::debug("EngineDoctorCore: Initializing all registered modules.");
    module_manager_.initialize_all(config_);
}

void EngineDoctorCore::perform_scan(const ScanParameters& params) {
    Logger::info("EngineDoctorCore: Performing scan.");
    // Получаем экземпляр сканера из менеджера модулей
    auto scanner = context_.get_module<ScannerModule>();
    if (!scanner) {
        throw ModuleNotFoundError("ScannerModule");
    }
    scanner->scan(params);
    Logger::info("EngineDoctorCore: Scan complete.");
}

void EngineDoctorCore::perform_analysis() {
    Logger::info("EngineDoctorCore: Performing analysis.");
    auto analyzer = context_.get_module<AnalysisEngine>();
    if (!analyzer) {
        throw ModuleNotFoundError("AnalysisEngine");
    }
    analyzer->analyze();
    Logger::info("EngineDoctorCore: Analysis complete.");
}

void EngineDoctorCore::generate_report() {
    Logger::info("EngineDoctorCore: Generating report.");
    auto reporter = context_.get_module<ReportGeneratorInterface>();
    if (!reporter) {
        Logger::warning("EngineDoctorCore: ReportGeneratorInterface is not registered, skipping report generation.");
        return;
    }
    reporter->generate();
    Logger::info("EngineDoctorCore: Report generation complete.");
}

} // namespace EngineDoctor
