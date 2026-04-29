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
    : config_(NormalizeConfig(config)),
            context_(),
            module_manager_(context_) {
	context_.set_data(kContextConfigDataKey, config_);
    // Инициализация логгера с настройками из конфигурации
    Logger::initialize(config_.log_level);
    Logger::info("EngineDoctorCore: Initializing...");
	if (config_.publish_events) {
		context_.event_bus().publish(
			"doctor.core.initialized",
			"EngineDoctorCore initialized",
			{{"config", BuildConfigSummary(config_)}});
	}

    // Создание и регистрация модулей через ModuleManager
    module_manager_.register_module(std::make_unique<ScannerModule>(context_));
    module_manager_.register_module(std::make_unique<AnalysisEngine>(context_));
    // ... регистрация других модулей

    Logger::info("EngineDoctorCore: Initialization complete.");
}

EngineDoctorCore::~EngineDoctorCore() {
    Logger::info("EngineDoctorCore: Shutting down...");
    if (config_.publish_events) {
        context_.event_bus().publish("doctor.core.shutdown", "EngineDoctorCore shutting down");
    }
    // Очистка ресурсов, освобождение памяти
    Logger::info("EngineDoctorCore: Shutdown complete.");
}

void EngineDoctorCore::run(const ScanParameters& params) {
    Logger::info("EngineDoctorCore: Starting run with parameters.");
    if (config_.publish_events) {
        context_.event_bus().publish(
            "doctor.core.run.started",
            "EngineDoctorCore run started",
            {{"path_count", std::to_string(params.paths_to_scan.size())}});
    }
    initialize_modules();
    perform_scan(params);
    perform_analysis();
    generate_report();
    Logger::info("EngineDoctorCore: Run finished.");
    if (config_.publish_events) {
        context_.event_bus().publish("doctor.core.run.finished", "EngineDoctorCore run finished");
    }
}

void EngineDoctorCore::initialize_modules() {
    Logger::debug("EngineDoctorCore: Initializing all registered modules.");
    module_manager_.initialize_all(config_);
    if (config_.publish_events) {
        context_.event_bus().publish("doctor.core.modules.initialized", "EngineDoctorCore modules initialized");
    }
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
	if (config_.publish_events) {
		context_.event_bus().publish("doctor.core.scan.completed", "EngineDoctorCore scan completed");
	}
}

void EngineDoctorCore::perform_analysis() {
    Logger::info("EngineDoctorCore: Performing analysis.");
    auto analyzer = context_.get_module<AnalysisEngine>();
    if (!analyzer) {
        throw ModuleNotFoundError("AnalysisEngine");
    }
    analyzer->analyze();
    Logger::info("EngineDoctorCore: Analysis complete.");
	if (config_.publish_events) {
		context_.event_bus().publish("doctor.core.analysis.completed", "EngineDoctorCore analysis completed");
	}
}

void EngineDoctorCore::generate_report() {
    Logger::info("EngineDoctorCore: Generating report.");
    auto reporter = context_.get_module<ReportGeneratorInterface>();
    if (!reporter) {
        Logger::warning("EngineDoctorCore: ReportGeneratorInterface is not registered, skipping report generation.");
        if (config_.publish_events) {
            context_.event_bus().publish("doctor.core.report.skipped", "ReportGeneratorInterface not registered");
        }
        return;
    }
    reporter->generate();
    Logger::info("EngineDoctorCore: Report generation complete.");
    if (config_.publish_events) {
        context_.event_bus().publish("doctor.core.report.completed", "EngineDoctorCore report generation completed");
    }
}

Context& EngineDoctorCore::context() {
    return context_;
}

const Context& EngineDoctorCore::context() const {
    return context_;
}

const Config& EngineDoctorCore::config() const {
    return config_;
}

} // namespace EngineDoctor
