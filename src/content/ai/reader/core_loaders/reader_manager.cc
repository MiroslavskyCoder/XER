#include "reader_manager.h"

#include "reader_factory.h"
#include "reader_registry.h"

namespace Engine::ModelsBuilder::Reader {

ReaderManager::ReaderManager() {
	ReaderFactory::RegisterDefaults();
}

LoadResult ReaderManager::Load(const std::string& filepath) {
	std::unique_ptr<ReaderBase> reader = CreateReader(filepath);
	if (!reader) {
		return LoadResult{nullptr, false, "No reader registered for: " + filepath};
	}
	return reader->Load(filepath);
}

std::unique_ptr<ReaderBase> ReaderManager::CreateReader(const std::string& filepath) const {
	const std::string extension = ExtractExtension(filepath);
	if (extension.empty()) {
		return nullptr;
	}
	return ReaderRegistry::GetInstance().Create(extension);
}

std::string ReaderManager::ExtractExtension(const std::string& filepath) {
	const size_t dot = filepath.find_last_of('.');
	if (dot == std::string::npos) {
		return {};
	}
	return filepath.substr(dot);
}

}  // namespace Engine::ModelsBuilder::Reader

