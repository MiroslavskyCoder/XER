/**
 * @file dependency_analyzer.cc
 * @brief Реализация анализа зависимостей.
 * @author Yoshi Arasaka (main)
 * @author LXXV (contributor)
 * @date 2023-10-27
 * @copyright MIT License
 */

#include "analysis/dependency_analyzer.h"
#include "core/logger.h"
#include "core/exceptions.h"
#include "system_analysis/os_detection_manager.h"
#include "package_handling/package_fetcher.h"
#include "v8_integration/v8_runtime_checker.h"

namespace EngineDoctor {

DependencyAnalyzer::DependencyAnalyzer(Context& context) : context_(context) {}

void DependencyAnalyzer::analyze_project_dependencies(DependencyGraph& graph) {
    Logger::info("DependencyAnalyzer: Analyzing project dependencies.");

    // Получаем список всех файлов для анализа (например, из результата сканирования)
    auto scan_result_data = context_.get_data<std::vector<ScanResult>>("scan_results");
    if (!scan_result_data) {
        Logger::warning("DependencyAnalyzer: No scan results found, cannot analyze project dependencies.");
        return;
    }

    for (const auto& scan_result : *scan_result_data) {
        if (scan_result.status == ScanStatus::SUCCESS) {
            std::vector<FileDependencyInfo> file_deps;
            analyze_file_dependencies(scan_result.file_path, file_deps);
            graph.add_file_dependencies(scan_result.file_path, file_deps);
        }
    }

    Logger::info("DependencyAnalyzer: Project dependencies analyzed.");
}

void DependencyAnalyzer::analyze_file_dependencies(const std::string& file_path, std::vector<FileDependencyInfo>& dependencies) {
    Logger::debug("DependencyAnalyzer: Analyzing dependencies for file: %s", file_path.c_str());

    // Проверка кэша
    if (file_dependencies_cache_.count(file_path)) {
        dependencies = file_dependencies_cache_[file_path];
        return;
    }

    // Реальная логика парсинга файла (например, поиск #include, вызовов функций)
    // Для простоты, здесь заглушки:
    try {
        parse_includes(file_path, dependencies);
        parse_function_calls(file_path, dependencies);
        // ... другие типы анализа

        file_dependencies_cache_[file_path] = dependencies;
        Logger::debug("DependencyAnalyzer: Dependencies analyzed for %s", file_path.c_str());
    } catch (const std::exception& e) {
        Logger::error("DependencyAnalyzer: Error analyzing dependencies for %s: %s", file_path.c_str(), e.what());
        // Здесь можно добавить запись об ошибке в результат сканирования
    }
}

void DependencyAnalyzer::analyze_system_and_library_dependencies(
    std::vector<LibraryDependencyInfo>& system_libs,
    std::vector<std::string>& v8_conflicts
) {
    Logger::info("DependencyAnalyzer: Analyzing system and library dependencies.");

    // 1. Определение операционной системы
    auto os_detector_manager = context_.get_module<OSDetectionManager>();
    std::string os_name = "unknown";
    if (os_detector_manager) {
        os_name = os_detector_manager->detect_os();
    }

    // 2. Получение списка установленных библиотек (специфично для ОС)
    std::vector<LibraryDependencyInfo> found_libs;
    if (os_name == "linux") {
        // Используем Linux-специфичные инструменты или PackageFetcher
        auto package_fetcher = context_.get_module<PackageFetcher>();
        if (package_fetcher) {
            package_fetcher->fetch_installed_packages(found_libs, os_name);
        }
    } else if (os_name == "windows") {
        // ... для Windows
    }
    // ... другие ОС

    system_libs = found_libs; // Копируем найденные библиотеки

    // 3. Анализ конфликтов с V8
    auto v8_checker = context_.get_module<V8RuntimeChecker>();
    if (v8_checker) {
        v8_checker->check_v8_compatibility(found_libs, v8_conflicts);
    } else {
        Logger::warning("V8RuntimeChecker not found, skipping V8 conflict analysis.");
    }

    Logger::info("DependencyAnalyzer: System and library dependencies analyzed.");
}

// --- Заглушки методов парсинга --- 

void DependencyAnalyzer::parse_includes(const std::string& file_path, std::vector<FileDependencyInfo>& dependencies) {
    Logger::debug("DependencyAnalyzer: Parsing includes for %s", file_path.c_str());
    // Реальная логика: чтение файла, поиск #include, добавление в dependencies.
    // Пример:
    if (file_path.find("header_a.h") != std::string::npos) {
        dependencies.push_back({"header_a.h", DependencyType::INCLUDE, ""});
    }
}

void DependencyAnalyzer::parse_function_calls(const std::string& file_path, std::vector<FileDependencyInfo>& dependencies) {
    Logger::debug("DependencyAnalyzer: Parsing function calls for %s", file_path.c_str());
    // Реальная логика: парсинг AST, поиск вызовов функций.
    // Пример:
    if (file_path.find("main.cc") != std::string::npos) {
        dependencies.push_back({"other_module.cc", DependencyType::FUNCTION_CALL, "some_function"});
    }
}

// ... другие методы парсинга

} // namespace EngineDoctor
