#include "reader_pipeline.h"

#include "reader_manager.h"

namespace Engine::ModelsBuilder::Reader {

LoadResult ReaderPipeline::Process(const std::string& filepath) {
	ReaderManager manager;
	return manager.Load(filepath);
}

}  // namespace Engine::ModelsBuilder::Reader

