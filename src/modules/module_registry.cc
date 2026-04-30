#include "modules/module_registry_internal.h"

#include <cctype>
#include <string>
#include <vector>

namespace modules {
namespace {

std::string NormalizeModuleKey(const std::string& text) {
	std::string out;
	out.reserve(text.size());
	for (unsigned char ch : text) {
		if (std::isalnum(ch) != 0) {
			out.push_back(static_cast<char>(std::tolower(ch)));
		}
	}
	return out;
}

}  // namespace

std::string ResolveCanonicalModuleName(const std::string& module_name) {
	const std::string normalized = NormalizeModuleKey(module_name);
	if (normalized == "container" || normalized == "project" || normalized == "sourcetree") {
		return "Container";
	}
	if (normalized == "system") {
		return "System";
	}
	if (normalized == "network") {
		return "Network";
	}
	if (normalized == "audio") {
		return "Audio";
	}
	if (normalized == "git") {
		return "Git";
	}
	if (normalized == "crypto") {
		return "Crypto";
	}
	if (normalized == "doctor") {
		return "Doctor";
	}
	if (normalized == "filesystem" || normalized == "fs") {
		return "FileSystem";
	}
	if (normalized == "ioasync") {
		return "IOAsync";
	}
	if (normalized == "util" || normalized == "utils") {
		return "Util";
	}
	if (normalized == "provider") {
		return "Provider";
	}
	if (normalized == "runtimelive" || normalized == "live") {
		return "RuntimeLive";
	}
	if (normalized == "opencv") {
		return "OpenCV";
	}
	if (normalized == "cuda") {
		return "CUDA";
	}
	if (normalized == "cudnn") {
		return "CUDNN";
	}
	if (normalized == "skia") {
		return "Skia";
	}
	if (normalized == "ffmpeg") {
		return "FFmpeg";
	}
	if (normalized == "angle") {
		return "ANGLE";
	}
	if (normalized == "vtk") {
		return "VTK";
	}
	return std::string();
}

std::vector<std::string> ListModules() {
	return {
		"Container",
		"System",
		"Network",
		"Audio",
		"Git",
		"Crypto",
		"FileSystem",
		"IO/Async",
		"Provider",
		"RuntimeLive",
		"Util",
		"Doctor",
		"OpenCV",
		"CUDA",
		"CUDNN",
		"Skia",
		"FFmpeg",
		"ANGLE",
		"VTK",
	};
}

bool ImportModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  const std::string& module_name,
			  std::string* error_out) {
	const std::string canonical_name = ResolveCanonicalModuleName(module_name);
	if (canonical_name.empty()) {
		if (error_out != nullptr) {
			*error_out = "unknown module: " + module_name;
		}
		return false;
	}

	v8::Local<v8::Object> module;
	bool ok = false;
	if (canonical_name == "Container") {
		ok = detail::BuildContainerModule(isolate, context, &module, error_out);
	} else if (canonical_name == "System") {
		ok = detail::BuildSystemModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Network") {
		ok = detail::BuildNetworkModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Audio") {
		ok = detail::BuildAudioModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Git") {
		ok = detail::BuildGitModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Crypto") {
		ok = detail::BuildCryptoModule(isolate, context, &module, error_out);
	} else if (canonical_name == "FileSystem") {
		ok = detail::BuildFileSystemModule(isolate, context, &module, error_out);
	} else if (canonical_name == "IOAsync") {
		ok = detail::BuildIOAsyncModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Util") {
		ok = detail::BuildUtilModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Provider") {
		ok = detail::BuildProviderModule(isolate, context, &module, error_out);
	} else if (canonical_name == "RuntimeLive") {
		ok = detail::BuildRuntimeLiveModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Doctor") {
		ok = detail::BuildDoctorModule(isolate, context, &module, error_out);
	} else if (canonical_name == "OpenCV") {
		ok = detail::BuildOpenCvModule(isolate, context, &module, error_out);
	} else if (canonical_name == "CUDA") {
		ok = detail::BuildCudaModule(isolate, context, &module, error_out);
	} else if (canonical_name == "CUDNN") {
		ok = detail::BuildCudnnModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Skia") {
		ok = detail::BuildSkiaModule(isolate, context, &module, error_out);
	} else if (canonical_name == "FFmpeg") {
		ok = detail::BuildFFmpegModule(isolate, context, &module, error_out);
	} else if (canonical_name == "ANGLE") {
		ok = detail::BuildAngleModule(isolate, context, &module, error_out);
	} else if (canonical_name == "VTK") {
		ok = detail::BuildVtkModule(isolate, context, &module, error_out);
	}

	if (!ok) {
		if (error_out != nullptr && error_out->empty()) {
			*error_out = "failed to initialize module: " + canonical_name;
		}
		return false;
	}

	if (!Engine::Helper::ExportGlobalModule(isolate, context, canonical_name.c_str(), module)) {
		if (error_out != nullptr) {
			*error_out = "failed to export module to global scope: " + canonical_name;
		}
		return false;
	}

	// --- PXER: экспортировать все JS-модули, зарегистрированные плагинами ---
	{
		auto pxer_js_modules = Engine::Native::Plugin::Pxer::PluginHost::Shared().GetJsModuleBuilders();
		for (const auto& entry : pxer_js_modules) {
			const std::string& name = entry.first;
			void* builder_fn = entry.second;
			using BuilderFn = int(*)(void*, void*, void*, void*);
			BuilderFn fn = reinterpret_cast<BuilderFn>(builder_fn);
			v8::Local<v8::Object> pxer_module = v8::Object::New(isolate);
			int ok = fn(isolate, *context, *pxer_module, nullptr);
			if (ok) {
				Engine::Helper::ExportGlobalModule(isolate, context, name.c_str(), pxer_module);
			}
		}
	}
	return true;
}

}  // namespace modules