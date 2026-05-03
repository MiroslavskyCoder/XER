#include "reader_factory.h"

#include "reader_registry.h"
#include "../framework_adapters/caffe_prototxt_reader.h"
#include "../framework_adapters/keras_h5_reader.h"
#include "../framework_adapters/pt_model_parser.h"
#include "../framework_adapters/tf_pb_reader.h"
#include "../framework_adapters/tf_saved_model_parser.h"

namespace Engine::ModelsBuilder::Reader {

void ReaderFactory::RegisterDefaults() {
	auto& registry = ReaderRegistry::GetInstance();
	registry.Register(".pb", []() {
		return std::make_unique<Framework::TfPbReader>();
	});
	registry.Register(".pt", []() {
		return std::make_unique<Framework::PtModelParser>();
	});
	registry.Register(".pth", []() {
		return std::make_unique<Framework::PtModelParser>();
	});
	registry.Register(".h5", []() {
		return std::make_unique<Framework::KerasH5Reader>();
	});
	registry.Register(".keras", []() {
		return std::make_unique<Framework::KerasH5Reader>();
	});
	registry.Register(".prototxt", []() {
		return std::make_unique<Framework::CaffeProtoxtReader>();
	});
}

}  // namespace Engine::ModelsBuilder::Reader

