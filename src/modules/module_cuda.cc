#include "modules/module_bridge_common.h"
#include "modules/module_builders.h"

#include "wrapper/cuda/cuda_engine_bridge.h"

namespace modules::detail {

bool BuildCudaModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out) {
	return BuildBridgeStatusModule(
		isolate,
		context,
		"CUDA",
		engine::bridge::cuda::IsAvailable(),
		engine::bridge::cuda::Summary(),
		module_out,
		error_out);
}

}  // namespace modules::detail