#pragma once

#include "reader_base.h"

#include <string>

namespace Engine::ModelsBuilder::Reader {

class ReaderPipeline {
 public:
	static LoadResult Process(const std::string& filepath);
};

}  // namespace Engine::ModelsBuilder::Reader

