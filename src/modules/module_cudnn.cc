#include "modules/module_bridge_common.h"
#include "modules/module_builders.h"

#if ENGINE_HAS_CUDNN_BRIDGE
#include "wrapper/cudnn/cudnn_engine_bridge.h"
#endif

namespace modules::detail {

bool BuildCudnnModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	return BuildBridgeStatusModule(
		isolate,
		context,
		"CUDNN",
#if ENGINE_HAS_CUDNN_BRIDGE
		engine::bridge::cudnn::IsAvailable(),
		engine::bridge::cudnn::Summary(),
#else
		false,
		"cuDNN bridge disabled by ENABLE_CUDNN=OFF",
#endif
		module_out,
		error_out);
}

}  // namespace modules::detail