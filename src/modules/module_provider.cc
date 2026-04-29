#include "modules/module_builders.h"

#include "provider.h"

#include <string>
#include <utility>

namespace modules::detail {
namespace {

Provider* UnwrapProviderStore(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if (!args.This()->IsObject()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "ProviderStore receiver is invalid");
		return nullptr;
	}
	Provider* provider = Engine::Helper::UnwrapPointer<Provider>(args.This().As<v8::Object>());
	if (provider == nullptr) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "ProviderStore is not initialized");
	}
	return provider;
}

void PopulateProviderFromObject(v8::Isolate* isolate,
				v8::Local<v8::Context> context,
				Provider* provider,
				v8::Local<v8::Object> object) {
	v8::Local<v8::Array> keys;
	if (!object->GetOwnPropertyNames(context).ToLocal(&keys)) {
		return;
	}
	for (uint32_t index = 0; index < keys->Length(); ++index) {
		v8::Local<v8::Value> key;
		v8::Local<v8::Value> value;
		if (!keys->Get(context, index).ToLocal(&key) || !object->Get(context, key).ToLocal(&value)) {
			continue;
		}
		provider->set(Engine::Helper::FromV8Str(isolate, key), Engine::Helper::FromV8Str(isolate, value));
	}
}

v8::Local<v8::FunctionTemplate> MakeProviderStoreTemplate(v8::Isolate* isolate);

void ProviderStoreConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (!args.IsConstructCall()) {
		v8::Local<v8::Function> constructor = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
		v8::Local<v8::Value> initial_value = args.Length() > 0
			? v8::Local<v8::Value>(args[0])
			: v8::Local<v8::Value>(v8::Undefined(isolate));
		v8::Local<v8::Value> argv[1] = {initial_value};
		v8::Local<v8::Object> instance = constructor->NewInstance(context, args.Length() > 0 ? 1 : 0, argv).ToLocalChecked();
		args.GetReturnValue().Set(instance);
		return;
	}

	auto* provider = new Provider();
	if (args.Length() > 0 && args[0]->IsObject()) {
		PopulateProviderFromObject(isolate, context, provider, args[0].As<v8::Object>());
	}
	Engine::Helper::WrapPointer(args.This(), provider);
	Engine::Helper::RegisterWeakCleanup(isolate, args.This(), provider);
	args.GetReturnValue().Set(args.This());
}

void ProviderStoreSetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	std::string key;
	std::string value;
	if (!RequireStringArg(args, 0, "ProviderStore.set expects key string", &key)
		|| !RequireStringArg(args, 1, "ProviderStore.set expects value string", &value)) {
		return;
	}
	provider->set(std::move(key), std::move(value));
	args.GetReturnValue().Set(args.This());
}

void ProviderStoreGetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	std::string key;
	if (!RequireStringArg(args, 0, "ProviderStore.get expects key string", &key)) {
		return;
	}
	const std::string* value = provider->get(key);
	if (value != nullptr) {
		args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), *value));
		return;
	}
	if (args.Length() > 1) {
		args.GetReturnValue().Set(args[1]);
		return;
	}
	args.GetReturnValue().Set(v8::Undefined(args.GetIsolate()));
}

void ProviderStoreKeysCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	args.GetReturnValue().Set(MakeStringArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), provider->keys()));
}

v8::Local<v8::FunctionTemplate> MakeProviderStoreTemplate(v8::Isolate* isolate) {
	return Engine::Helper::MakeClass(
		isolate,
		"ProviderStore",
		&ProviderStoreConstructor,
		{{"set", &ProviderStoreSetCallback},
		 {"get", &ProviderStoreGetCallback},
		 {"keys", &ProviderStoreKeysCallback}});
}

void ProviderCreateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Function> constructor = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
	v8::Local<v8::Value> initial_value = args.Length() > 0
		? v8::Local<v8::Value>(args[0])
		: v8::Local<v8::Value>(v8::Undefined(isolate));
	v8::Local<v8::Value> argv[1] = {initial_value};
	v8::Local<v8::Object> instance = constructor->NewInstance(context, args.Length() > 0 ? 1 : 0, argv).ToLocalChecked();
	args.GetReturnValue().Set(instance);
}

}  // namespace

bool BuildProviderModule(v8::Isolate* isolate,
			     v8::Local<v8::Context> context,
			     v8::Local<v8::Object>* module_out,
			     std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	v8::Local<v8::Function> provider_store = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
	bool ok = true;
	ok = ok && SetProperty(isolate, context, module, "ProviderStore", provider_store);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "create", &ProviderCreateCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Provider module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail