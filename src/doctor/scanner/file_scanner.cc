/**
 * @file file_scanner.cc
 * @brief Реализация интерфейса для сканирования отдельных файлов.
 * @author LXXV (main)
 * @author byMiroslavsky (contributor)
 * @date 2023-10-27
 * @copyright MIT License
 */

#include "scanner/file_scanner.h"
#include "core/logger.h"
#include "scanner/file_metadata_extractor.h"
#include "analysis/dependency_analyzer.h"

namespace EngineDoctor {

FileScanner::FileScanner(Context& context) : context_(context) {}

CppFileScanner::CppFileScanner(Context& context) : FileScanner(context) {}

ScanResult CppFileScanner::scan_file(const std::string& file_path, const ScanParameters& params) {
    Logger::debug("CppFileScanner: Scanning file: %s", file_path.c_str());
    ScanResult result;
    result.file_path = file_path;

    try {
        // 1. Извлечение метаданных файла
        FileMetadataExtractor metadata_extractor;
        result.metadata = metadata_extractor.extract(file_path);

        // 2. Анализ синтаксиса (если включено в параметрах)
        if (params.analyze_syntax) {
            analyze_syntax(file_path, result);
        }

        // 3. Анализ зависимостей (если включено)
        if (params.analyze_dependencies) {
            auto dep_analyzer = context_.get_module<DependencyAnalyzer>();
            if (dep_analyzer) {
                dep_analyzer->analyze_file_dependencies(file_path, result.dependencies);
            } else {
                Logger::warning("DependencyAnalyzer not found, skipping dependency analysis for %s", file_path.c_str());
            }
        }

        // 4. Дополнительные анализы...

        result.status = ScanStatus::SUCCESS;
        Logger::debug("CppFileScanner: File scanned successfully: %s", file_path.c_str());
    } catch (const std::exception& e) {
        Logger::error("CppFileScanner: Error scanning file %s: %s", file_path.c_str(), e.what());
        result.status = ScanStatus::ERROR;
        result.error_message = e.what();
    }

    return result;
}

void CppFileScanner::analyze_syntax(const std::string& file_path, ScanResult& result) {
    Logger::debug("CppFileScanner: Analyzing syntax for %s", file_path.c_str());
    // Здесь будет логика парсинга C++ кода, проверки на синтаксические ошибки.
    // Это может включать использование внешних библиотек или собственный парсер.
    // На данный момент, заглушка:
    if (file_path.find(".invalid") != std::string::npos) {
        result.errors.emplace_back("SYNTAX_ERROR", "Invalid syntax detected.");
    }
}

void CppFileScanner::analyze_dependencies(const std::string& file_path, ScanResult& result) {
    Logger::debug("CppFileScanner: Analyzing dependencies for %s", file_path.c_str());
    // Здесь будет логика поиска #include директив и анализа использования внешних функций/классов.
    // На данный момент, заглушка:
    if (file_path.find("missing_header") != std::string::npos) {
        result.warnings.emplace_back("DEPENDENCY_WARNING", "Potentially missing header included.");
    }
}

} // namespace EngineDoctor
