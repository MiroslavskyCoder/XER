#pragma once

#include <string>

#include <v8.h>

namespace flux::console {

std::string SnapshotFromArgs(v8::Isolate* isolate,
			     const v8::FunctionCallbackInfo<v8::Value>& args,
			     int stream_arg_index = 0);

void ClearSnapshotFromArgs(v8::Isolate* isolate,
			   const v8::FunctionCallbackInfo<v8::Value>& args,
			   int stream_arg_index = 0);

}  // namespace flux::console