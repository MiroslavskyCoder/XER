/**
 * @file file_scanner.h
 * @brief Интерфейс для сканирования отдельных файлов.
 * @author LXXV (main)
 * @author byMiroslavsky (contributor)
 * @date 2023-10-27
 * @copyright MIT License
 */

#pragma once

#include <string>
#include <vector>
#include "scanner/scan_parameters.h"
#include "scanner/scan_result.h"
#include "core/engine_doctor_context.h"

namespace EngineDoctor {

class FileScanner {
public:
    FileScanner(Context& context);
    virtual ~FileScanner() = default;

    /**
     * @brief Сканирует указанный файл.
     * @param file_path Путь к файлу.
     * @param params Параметры сканирования.
     * @return Результат сканирования файла.
     */
    virtual ScanResult scan_file(const std::string& file_path, const ScanParameters& params) = 0;

protected:
    Context& context_;
    // Дополнительные члены класса...
};

// Пример конкретной реализации
class CppFileScanner : public FileScanner {
public:
    CppFileScanner(Context& context);

    ScanResult scan_file(const std::string& file_path, const ScanParameters& params) override;

private:
    // Методы для специфического анализа C++ файлов
    void analyze_syntax(const std::string& file_path, ScanResult& result);
    void analyze_dependencies(const std::string& file_path, ScanResult& result);
    // ...
};

} // namespace EngineDoctor
