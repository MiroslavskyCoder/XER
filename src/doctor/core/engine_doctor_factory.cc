#include "core/engine_doctor_factory.h"

namespace EngineDoctor {

std::unique_ptr<FileScanner> EngineDoctorFactory::create_file_scanner(Context& context) {
	return std::make_unique<CppFileScanner>(context);
}

std::unique_ptr<DependencyAnalyzer> EngineDoctorFactory::create_dependency_analyzer(Context& context) {
	return std::make_unique<DependencyAnalyzer>(context);
}

} // namespace EngineDoctor
