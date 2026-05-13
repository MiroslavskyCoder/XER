#include "modules/module_bridge_common.h"
#include "modules/module_builders.h"

#if ENGINE_HAS_CUDA_BRIDGE
#include "wrapper/cuda/cuda_engine_bridge.h"
#endif

namespace modules::detail {

bool BuildCudaModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out) {
	return BuildBridgeStatusModule(
		isolate,
		context,
		"CUDA",
#if ENGINE_HAS_CUDA_BRIDGE
		engine::bridge::cuda::IsAvailable(),
		engine::bridge::cuda::Summary(),
#else
		false,
		"CUDA bridge disabled by ENABLE_CUDA=OFF",
#endif
		module_out,
		error_out);
}

}  // namespace modules::detail