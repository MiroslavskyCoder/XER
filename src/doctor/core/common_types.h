/**
 * @file common_types.h
 * @author byMiroslavsky (main)
 * @author LXXV (contributor)
 * @date 2026-10-27
 * @copyright MIT License
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <optional>

namespace doctor {

enum class Status {
    SUCCESS,       ///< Операция успешно завершена.
    FAILURE,       ///< Операция завершилась с ошибкой.
    IN_PROGRESS,   ///< Операция выполняется.
    PENDING,       ///< Операция ожидает выполнения.
    SKIPPED,       ///< Операция была пропущена.
    UNKNOWN        ///< Статус неизвестен.
};

enum class ScanObjectType {
    FILE,
    DIRECTORY,
    SYMLINK,
    OTHER
};

enum class ErrorCategory {
    NO_CORRECT,                 ///< Общая категория ошибок, не подпадающих под другие.
    NULL_EMPTY,                 ///< Обнаружены пустые или нулевые значения там, где не ожидалось.
    NO_0_FILE,                  ///< Файл или ресурс с нулевым размером, когда ожидается содержимое.
    FILE_NO_EMPTY,              ///< Файл, который должен быть пустым, но имеет содержимое.
    DIRECTORY,                  ///< Проблема, связанная с директорией.
    FILE_CALL_FAILED_FROM_RUNTIME,///< Ошибка вызова функции в рантайме.
    FATAL_STACK_ERROR,          ///< Критическая ошибка стека.
    V8_CONFLICT,                ///< Конфликт, связанный с V8.
    SYSTEM_DEPENDENCY_MISSING,    ///< Отсутствует системная зависимость.
    LIBRARY_VERSION_MISMATCH      ///< Несоответствие версий библиотек.
};

struct FileMetadata {
    std::string full_path;               ///< Полный путь к файлу.
    ScanObjectType type = ScanObjectType::OTHER; ///< Тип объекта.
    size_t size = 0;                     ///< Размер файла в байтах.
    std::chrono::system_clock::time_point last_modified; ///< Время последней модификации.
};

struct Problem {
    std::string code;                   ///< Уникальный код проблемы (например, "SYNTAX_ERROR", "V8_CONFLICT_LIBSSL").
    std::string message;                ///< Человекочитаемое сообщение об ошибке.
    ErrorCategory category = ErrorCategory::NO_CORRECT; ///< Категория проблемы.
    std::optional<std::string> offending_element; 
};
 
struct ScanResult {
    std::string file_path;              ///< Путь к сканированному файлу.
    FileMetadata metadata;              ///< Извлеченные метаданные файла.
    Status status = Status::UNKNOWN;    ///< Статус сканирования файла.
    std::string error_message;          ///< Сообщение об ошибке, если статус FAILURE или ERROR.
    std::vector<Problem> problems;      ///< Список обнаруженных проблем (ошибок, предупреждений).
}; 

struct DependencyInfo {
    std::string name;                   ///< Имя зависимости (библиотеки, файла).
    std::string version;                ///< Версия зависимости.
    std::string path;                   ///< Путь к зависимости (если применимо).
};
 
struct ScanParameters {
    std::vector<std::string> paths_to_scan; ///< Список путей для сканирования.
    bool analyze_syntax = true;             ///< Включить синтаксический анализ.
    bool analyze_dependencies = true;       ///< Включить анализ зависимостей.
    bool analyze_security = false;          ///< Включить анализ безопасности.
};
 
struct Config {
    int log_level = 1; 
    std::string report_output_path;
};
 
using Timestamp = std::chrono::system_clock::time_point; 
using ModuleId = std::string;

} // namespace doctor
