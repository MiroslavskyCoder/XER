#include "scanner/file_metadata_extractor.h"
#include <unicode/unistr.h>
#include <filesystem>

namespace EngineDoctor {

FileMetadata FileMetadataExtractor::extract(const std::string& file_path) const {
	FileMetadata meta;
	meta.full_path = file_path;

	// Используем ICU для нормализации Unicode-пути
	icu::UnicodeString u_path = icu::UnicodeString::fromUTF8(file_path);
	std::string norm_path;
	u_path.toUTF8String(norm_path);

	std::filesystem::path fs_path(norm_path);
	std::error_code ec;
	meta.exists = std::filesystem::exists(fs_path, ec);
	meta.is_directory = std::filesystem::is_directory(fs_path, ec);
	meta.size = meta.exists && !meta.is_directory ? std::filesystem::file_size(fs_path, ec) : 0;
	meta.extension = fs_path.has_extension() ? fs_path.extension().string() : std::string();

	return meta;
}

} // namespace EngineDoctor
