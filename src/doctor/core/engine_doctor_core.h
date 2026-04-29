/**
 * @file engine_doctor_core.h
 * @brief Основной класс EngineDoctor, управляющий всем процессом.
 * @author byMiroslavsky (main)
 * @author LXXV (contributor)
 * @date 2023-10-27
 * @copyright MIT License
 */

#pragma once

#include "core/engine_doctor_context.h"
#include "core/module_manager.h"
#include "core/engine_doctor_config.h"
#include "scanner/scanner_module.h"
#include "analysis/analysis_engine.h"

namespace EngineDoctor {

class EngineDoctorCore {
public:
    explicit EngineDoctorCore(const Config& config);
    ~EngineDoctorCore();

    void run(const ScanParameters& params);
	Context& context();
	const Context& context() const;
	const Config& config() const;

private:
    void initialize_modules();
    void perform_scan(const ScanParameters& params);
    void perform_analysis();
    void generate_report();

    Config config_;
    Context context_;
    ModuleManager module_manager_;

    // Дополнительные члены класса...
};

} // namespace EngineDoctor
