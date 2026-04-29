#include "modules/module_bridge_common.h"
#include "modules/module_builders.h"

#include "wrapper/opencv/opencv_engine_bridge.h"

namespace modules::detail {

bool BuildOpenCvModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	return BuildBridgeStatusModule(
		isolate,
		context,
		"OpenCV",
		engine::bridge::opencv::IsAvailable(),
		engine::bridge::opencv::Summary(),
		module_out,
		error_out);
}

}  // namespace modules::detail