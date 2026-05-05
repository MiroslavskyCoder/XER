#pragma once

#include "modules/module_common.h"

namespace modules::detail {

bool BuildContainerModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out);
bool BuildFileSystemModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out);
bool BuildUtilModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out);
bool BuildProviderModule(v8::Isolate* isolate,
			     v8::Local<v8::Context> context,
			     v8::Local<v8::Object>* module_out,
			     std::string* error_out);
bool BuildSystemModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out);
bool BuildNetworkModule(v8::Isolate* isolate,
			    v8::Local<v8::Context> context,
			    v8::Local<v8::Object>* module_out,
			    std::string* error_out);
bool BuildAudioModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out);
bool BuildGitModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out);
bool BuildCryptoModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out);
bool BuildIOAsyncModule(v8::Isolate* isolate,
			    v8::Local<v8::Context> context,
			    v8::Local<v8::Object>* module_out,
			    std::string* error_out);
bool BuildRuntimeLiveModule(v8::Isolate* isolate,
			    v8::Local<v8::Context> context,
			    v8::Local<v8::Object>* module_out,
			    std::string* error_out); 
bool BuildDeviceModule(v8::Isolate* isolate,
		       v8::Local<v8::Context> context,
		       v8::Local<v8::Object>* module_out,
		       std::string* error_out);
bool BuildOpenCvModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out);
bool BuildCudaModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out);
bool BuildCudnnModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out);
bool BuildSkiaModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out);
bool BuildFFmpegModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out); 
bool BuildAiModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out);
bool BuildSdModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out);
bool BuildImageModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out);

}  // namespace modules::detail