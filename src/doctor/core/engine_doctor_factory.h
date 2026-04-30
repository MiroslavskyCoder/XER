#pragma once

#include <memory>
#include "core/engine_doctor_context.h"
#include "scanner/file_scanner.h"
#include "analysis/dependency_analyzer.h"

namespace EngineDoctor {

class EngineDoctorFactory {
public:
	static std::unique_ptr<FileScanner> create_file_scanner(Context& context);
	static std::unique_ptr<DependencyAnalyzer> create_dependency_analyzer(Context& context);
	// Можно добавить фабрики для других компонентов
};

} // namespace EngineDoctor
