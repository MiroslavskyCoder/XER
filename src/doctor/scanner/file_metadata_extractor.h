#pragma once

#include <string>
#include "scanner/scan_result.h"
#include <unicode/unistr.h>
#include <filesystem>

namespace EngineDoctor {

class FileMetadataExtractor {
public:
	/**
	 * @brief Извлекает метаданные файла по пути (с поддержкой Unicode).
	 * @param file_path Путь к файлу (UTF-8).
	 * @return FileMetadata
	 */
	FileMetadata extract(const std::string& file_path) const;
};

} // namespace EngineDoctor
