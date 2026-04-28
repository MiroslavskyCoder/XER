/**
 * @file dependency_analyzer.h
 * @brief Анализирует зависимости между файлами и библиотеками.
 * @author Yoshi Arasaka (main)
 * @author LXXV (contributor)
 * @date 2023-10-27
 * @copyright MIT License
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include "analysis/dependency_graph.h"
#include "analysis/package_dependency.h"
#include "core/engine_doctor_context.h"

namespace EngineDoctor {

// Перечисление типов зависимостей
enum class DependencyType {
    INCLUDE,       // #include
    FUNCTION_CALL, // Вызов функции
    CLASS_USAGE,   // Использование класса
    LIBRARY_LINK   // Связывание с библиотекой
};

struct FileDependencyInfo {
    std::string target_file;
    DependencyType type;
    std::string detail; // Например, имя функции, класса, библиотеки
};

struct LibraryDependencyInfo {
    std::string name;
    std::string version;
    std::string path;
    std::vector<std::string> conflicts;
    // ... другие поля
};

class DependencyAnalyzer {
public:
    DependencyAnalyzer(Context& context);
    ~DependencyAnalyzer() = default;

    /**
     * @brief Анализирует зависимости всех файлов проекта.
     * @param graph Граф зависимостей для заполнения.
     */
    void analyze_project_dependencies(DependencyGraph& graph);

    /**
     * @brief Анализирует зависимости конкретного файла.
     * @param file_path Путь к файлу.
     * @param dependencies Список найденных зависимостей.
     */
    void analyze_file_dependencies(const std::string& file_path, std::vector<FileDependencyInfo>& dependencies);

    /**
     * @brief Анализирует установленные системные и библиотечные зависимости.
     * @param system_libs Список найденных системных библиотек.
     * @param v8_conflicts Список обнаруженных конфликтов с V8.
     */
    void analyze_system_and_library_dependencies(
        std::vector<LibraryDependencyInfo>& system_libs,
        std::vector<std::string>& v8_conflicts
    );

private:
    Context& context_;
    // Кэшированные данные для ускорения анализа
    std::map<std::string, std::vector<FileDependencyInfo>> file_dependencies_cache_;

    // Методы для парсинга и анализа различных типов зависимостей
    void parse_includes(const std::string& file_path, std::vector<FileDependencyInfo>& dependencies);
    void parse_function_calls(const std::string& file_path, std::vector<FileDependencyInfo>& dependencies);
    void detect_library_usage(const std::string& file_path, std::vector<LibraryDependencyInfo>& found_libs);

    // ...
};

} // namespace EngineDoctor
