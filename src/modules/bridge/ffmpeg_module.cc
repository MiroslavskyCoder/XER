#include "modules/bridge/ffmpeg_module.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"

namespace {

using engine::bridge::ffmpeg::BitstreamFilters;
using engine::bridge::ffmpeg::CodecInfo;
using engine::bridge::ffmpeg::DeviceInfo;
using engine::bridge::ffmpeg::FilterInfo;
using engine::bridge::ffmpeg::LibraryVersion;
using engine::bridge::ffmpeg::MediaInfo;
using engine::bridge::ffmpeg::MetadataEntry;
using engine::bridge::ffmpeg::NamedItem;
using engine::bridge::ffmpeg::PacketInfo;
using engine::bridge::ffmpeg::ProtocolInfo;
using engine::bridge::ffmpeg::StreamInfo;
using engine::bridge::ffmpeg::VideoFrameInfo;
using engine::bridge::ffmpeg::AudioFrameInfo;

v8::Local<v8::String> ToString(v8::Isolate* isolate, const std::string& value) {
    return v8::String::NewFromUtf8(isolate, value.c_str()).ToLocalChecked();
}

bool Set(v8::Local<v8::Context> context,
         v8::Local<v8::Object> object,
         const char* key,
         v8::Local<v8::Value> value) {
    return object
        ->Set(context,
              v8::String::NewFromUtf8(context->GetIsolate(), key).ToLocalChecked(),
              value)
        .FromMaybe(false);
}

bool ReadStringArg(const v8::FunctionCallbackInfo<v8::Value>& args,
                   int index,
                   std::string* out_value) {
    if (args.Length() <= index || !args[index]->IsString()) {
        return false;
    }
    v8::String::Utf8Value utf8(args.GetIsolate(), args[index]);
    *out_value = std::string(*utf8, utf8.length());
    return true;
}

void ThrowError(v8::Isolate* isolate, const std::string& message) {
    isolate->ThrowException(v8::Exception::Error(ToString(isolate, message)));
}

v8::Local<v8::Value> BytesToUint8Array(v8::Isolate* isolate,
                                       const std::vector<uint8_t>& bytes) {
    const size_t len = bytes.size();
    v8::Local<v8::ArrayBuffer> array_buffer = v8::ArrayBuffer::New(isolate, len);
    if (len > 0) {
        std::memcpy(array_buffer->GetBackingStore()->Data(), bytes.data(), len);
    }
    return v8::Uint8Array::New(array_buffer, 0, len);
}

v8::Local<v8::Array> IntVectorToArray(v8::Isolate* isolate,
                                      v8::Local<v8::Context> context,
                                      const std::vector<int>& values) {
    v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
    for (size_t i = 0; i < values.size(); ++i) {
        array->Set(context, static_cast<uint32_t>(i), v8::Integer::New(isolate, values[i])).FromMaybe(false);
    }
    return array;
}

v8::Local<v8::Object> MetadataToObject(v8::Isolate* isolate,
                                       v8::Local<v8::Context> context,
                                       const MetadataEntry& entry) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "key", ToString(isolate, entry.key));
    Set(context, object, "value", ToString(isolate, entry.value));
    return object;
}

v8::Local<v8::Object> VersionToObject(v8::Isolate* isolate,
                                      v8::Local<v8::Context> context,
                                      const LibraryVersion& version) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "name", ToString(isolate, version.name));
    Set(context, object, "version", v8::Number::New(isolate, version.version));
    Set(context, object, "major", v8::Integer::New(isolate, version.major));
    Set(context, object, "minor", v8::Integer::New(isolate, version.minor));
    Set(context, object, "micro", v8::Integer::New(isolate, version.micro));
    Set(context, object, "versionText", ToString(isolate, version.version_text));
    return object;
}

v8::Local<v8::Object> ProtocolToObject(v8::Isolate* isolate,
                                       v8::Local<v8::Context> context,
                                       const ProtocolInfo& protocol) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "name", ToString(isolate, protocol.name));
    Set(context, object, "input", v8::Boolean::New(isolate, protocol.input));
    Set(context, object, "output", v8::Boolean::New(isolate, protocol.output));
    return object;
}

v8::Local<v8::Object> NamedItemToObject(v8::Isolate* isolate,
                                        v8::Local<v8::Context> context,
                                        const NamedItem& item) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "name", ToString(isolate, item.name));
    Set(context, object, "description", ToString(isolate, item.description));
    return object;
}

v8::Local<v8::Object> CodecToObject(v8::Isolate* isolate,
                                    v8::Local<v8::Context> context,
                                    const CodecInfo& codec) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "name", ToString(isolate, codec.name));
    Set(context, object, "longName", ToString(isolate, codec.long_name));
    Set(context, object, "mediaType", ToString(isolate, codec.media_type));
    Set(context, object, "encoder", v8::Boolean::New(isolate, codec.encoder));
    Set(context, object, "decoder", v8::Boolean::New(isolate, codec.decoder));
    Set(context, object, "intraOnly", v8::Boolean::New(isolate, codec.intra_only));
    Set(context, object, "lossy", v8::Boolean::New(isolate, codec.lossy));
    Set(context, object, "lossless", v8::Boolean::New(isolate, codec.lossless));
    return object;
}

v8::Local<v8::Object> FilterToObject(v8::Isolate* isolate,
                                     v8::Local<v8::Context> context,
                                     const FilterInfo& filter) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "name", ToString(isolate, filter.name));
    Set(context, object, "description", ToString(isolate, filter.description));
    Set(context, object, "dynamicInputs", v8::Boolean::New(isolate, filter.dynamic_inputs));
    Set(context, object, "dynamicOutputs", v8::Boolean::New(isolate, filter.dynamic_outputs));
    Set(context, object, "sliceThreads", v8::Boolean::New(isolate, filter.slice_threads));
    Set(context, object, "timelineGeneric", v8::Boolean::New(isolate, filter.timeline_generic));
    Set(context, object, "timelineInternal", v8::Boolean::New(isolate, filter.timeline_internal));
    Set(context, object, "timelineSupport", v8::Boolean::New(isolate, filter.timeline_support));
    return object;
}

v8::Local<v8::Object> DeviceToObject(v8::Isolate* isolate,
                                     v8::Local<v8::Context> context,
                                     const DeviceInfo& device) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "name", ToString(isolate, device.name));
    Set(context, object, "description", ToString(isolate, device.description));
    Set(context, object, "mediaType", ToString(isolate, device.media_type));
    Set(context, object, "input", v8::Boolean::New(isolate, device.input));
    Set(context, object, "output", v8::Boolean::New(isolate, device.output));
    return object;
}

v8::Local<v8::Object> StreamToObject(v8::Isolate* isolate,
                                     v8::Local<v8::Context> context,
                                     const StreamInfo& stream_info) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "index", v8::Integer::New(isolate, stream_info.index));
    Set(context, object, "mediaType", ToString(isolate, stream_info.media_type));
    Set(context, object, "codecName", ToString(isolate, stream_info.codec_name));
    Set(context, object, "codecLongName", ToString(isolate, stream_info.codec_long_name));
    Set(context, object, "codecId", v8::Integer::New(isolate, stream_info.codec_id));
    Set(context, object, "width", v8::Integer::New(isolate, stream_info.width));
    Set(context, object, "height", v8::Integer::New(isolate, stream_info.height));
    Set(context, object, "sampleRate", v8::Integer::New(isolate, stream_info.sample_rate));
    Set(context, object, "channels", v8::Integer::New(isolate, stream_info.channels));
    Set(context, object, "channelLayout", v8::Number::New(isolate, static_cast<double>(stream_info.channel_layout)));
    Set(context, object, "sampleFormat", ToString(isolate, stream_info.sample_format));
    Set(context, object, "pixelFormat", ToString(isolate, stream_info.pixel_format));
    Set(context, object, "timeBaseNum", v8::Integer::New(isolate, stream_info.time_base_num));
    Set(context, object, "timeBaseDen", v8::Integer::New(isolate, stream_info.time_base_den));
    Set(context, object, "duration", v8::Number::New(isolate, static_cast<double>(stream_info.duration)));
    Set(context, object, "startTime", v8::Number::New(isolate, static_cast<double>(stream_info.start_time)));
    Set(context, object, "bitRate", v8::Number::New(isolate, static_cast<double>(stream_info.bit_rate)));
    Set(context, object, "frameCount", v8::Number::New(isolate, static_cast<double>(stream_info.frame_count)));
    Set(context, object, "language", ToString(isolate, stream_info.language));
    v8::Local<v8::Array> metadata = v8::Array::New(isolate, static_cast<int>(stream_info.metadata.size()));
    for (size_t i = 0; i < stream_info.metadata.size(); ++i) {
        metadata->Set(context, static_cast<uint32_t>(i),
                      MetadataToObject(isolate, context, stream_info.metadata[i])).FromMaybe(false);
    }
    Set(context, object, "metadata", metadata);
    return object;
}

v8::Local<v8::Object> MediaToObject(v8::Isolate* isolate,
                                    v8::Local<v8::Context> context,
                                    const MediaInfo& media) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "path", ToString(isolate, media.path));
    Set(context, object, "formatName", ToString(isolate, media.format_name));
    Set(context, object, "formatLongName", ToString(isolate, media.format_long_name));
    Set(context, object, "duration", v8::Number::New(isolate, static_cast<double>(media.duration)));
    Set(context, object, "startTime", v8::Number::New(isolate, static_cast<double>(media.start_time)));
    Set(context, object, "size", v8::Number::New(isolate, static_cast<double>(media.size)));
    Set(context, object, "bitRate", v8::Number::New(isolate, static_cast<double>(media.bit_rate)));
    Set(context, object, "streamCount", v8::Integer::New(isolate, media.stream_count));

    v8::Local<v8::Array> metadata = v8::Array::New(isolate, static_cast<int>(media.metadata.size()));
    for (size_t i = 0; i < media.metadata.size(); ++i) {
        metadata->Set(context, static_cast<uint32_t>(i),
                      MetadataToObject(isolate, context, media.metadata[i])).FromMaybe(false);
    }
    Set(context, object, "metadata", metadata);

    v8::Local<v8::Array> streams = v8::Array::New(isolate, static_cast<int>(media.streams.size()));
    for (size_t i = 0; i < media.streams.size(); ++i) {
        streams->Set(context, static_cast<uint32_t>(i),
                     StreamToObject(isolate, context, media.streams[i])).FromMaybe(false);
    }
    Set(context, object, "streams", streams);
    return object;
}

v8::Local<v8::Object> PacketToObject(v8::Isolate* isolate,
                                     v8::Local<v8::Context> context,
                                     const PacketInfo& packet) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "streamIndex", v8::Integer::New(isolate, packet.stream_index));
    Set(context, object, "mediaType", ToString(isolate, packet.media_type));
    Set(context, object, "pts", v8::Number::New(isolate, static_cast<double>(packet.pts)));
    Set(context, object, "dts", v8::Number::New(isolate, static_cast<double>(packet.dts)));
    Set(context, object, "duration", v8::Number::New(isolate, static_cast<double>(packet.duration)));
    Set(context, object, "pos", v8::Number::New(isolate, static_cast<double>(packet.pos)));
    Set(context, object, "size", v8::Integer::New(isolate, packet.size));
    Set(context, object, "keyFrame", v8::Boolean::New(isolate, packet.key_frame));
    Set(context, object, "corrupt", v8::Boolean::New(isolate, packet.corrupt));
    return object;
}

v8::Local<v8::Object> VideoFrameToObject(v8::Isolate* isolate,
                                         v8::Local<v8::Context> context,
                                         const VideoFrameInfo& frame) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "streamIndex", v8::Integer::New(isolate, frame.stream_index));
    Set(context, object, "width", v8::Integer::New(isolate, frame.width));
    Set(context, object, "height", v8::Integer::New(isolate, frame.height));
    Set(context, object, "pixelFormat", ToString(isolate, frame.pixel_format));
    Set(context, object, "pictureType", ToString(isolate, frame.picture_type));
    Set(context, object, "pts", v8::Number::New(isolate, static_cast<double>(frame.pts)));
    Set(context, object, "bestEffortTimestamp", v8::Number::New(isolate, static_cast<double>(frame.best_effort_timestamp)));
    Set(context, object, "duration", v8::Number::New(isolate, static_cast<double>(frame.duration)));
    Set(context, object, "keyFrame", v8::Boolean::New(isolate, frame.key_frame));
    Set(context, object, "lineSizes", IntVectorToArray(isolate, context, frame.line_sizes));
    Set(context, object, "data", BytesToUint8Array(isolate, frame.data));
    return object;
}

v8::Local<v8::Object> AudioFrameToObject(v8::Isolate* isolate,
                                         v8::Local<v8::Context> context,
                                         const AudioFrameInfo& frame) {
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    Set(context, object, "streamIndex", v8::Integer::New(isolate, frame.stream_index));
    Set(context, object, "sampleRate", v8::Integer::New(isolate, frame.sample_rate));
    Set(context, object, "channels", v8::Integer::New(isolate, frame.channels));
    Set(context, object, "sampleCount", v8::Integer::New(isolate, frame.sample_count));
    Set(context, object, "planar", v8::Boolean::New(isolate, frame.planar));
    Set(context, object, "sampleFormat", ToString(isolate, frame.sample_format));
    Set(context, object, "pts", v8::Number::New(isolate, static_cast<double>(frame.pts)));
    Set(context, object, "bestEffortTimestamp", v8::Number::New(isolate, static_cast<double>(frame.best_effort_timestamp)));
    Set(context, object, "planeSizes", IntVectorToArray(isolate, context, frame.plane_sizes));
    Set(context, object, "data", BytesToUint8Array(isolate, frame.data));
    return object;
}

template <typename T, typename Converter>
v8::Local<v8::Array> VectorToArray(v8::Isolate* isolate,
                                   v8::Local<v8::Context> context,
                                   const std::vector<T>& values,
                                   Converter converter) {
    v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
    for (size_t i = 0; i < values.size(); ++i) {
        array->Set(context, static_cast<uint32_t>(i), converter(values[i])).FromMaybe(false);
    }
    return array;
}

void VersionsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::LibraryVersions();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const LibraryVersion& value) { return VersionToObject(isolate, context, value); }));
}

void ConfigurationCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(ToString(args.GetIsolate(), engine::bridge::ffmpeg::Configuration()));
}

void LicenseCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(ToString(args.GetIsolate(), engine::bridge::ffmpeg::License()));
}

void ProtocolsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::Protocols();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const ProtocolInfo& value) { return ProtocolToObject(isolate, context, value); }));
}

void InputFormatsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::InputFormats();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const NamedItem& value) { return NamedItemToObject(isolate, context, value); }));
}

void OutputFormatsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::OutputFormats();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const NamedItem& value) { return NamedItemToObject(isolate, context, value); }));
}

void CodecsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::Codecs();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const CodecInfo& value) { return CodecToObject(isolate, context, value); }));
}

void EncodersCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    auto values = engine::bridge::ffmpeg::Codecs();
    values.erase(std::remove_if(values.begin(), values.end(),
                                [](const CodecInfo& item) { return !item.encoder; }),
                 values.end());
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const CodecInfo& value) { return CodecToObject(isolate, context, value); }));
}

void DecodersCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    auto values = engine::bridge::ffmpeg::Codecs();
    values.erase(std::remove_if(values.begin(), values.end(),
                                [](const CodecInfo& item) { return !item.decoder; }),
                 values.end());
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const CodecInfo& value) { return CodecToObject(isolate, context, value); }));
}

void BitstreamFiltersCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::BitstreamFilters();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const NamedItem& value) { return NamedItemToObject(isolate, context, value); }));
}

void FiltersCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::Filters();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const FilterInfo& value) { return FilterToObject(isolate, context, value); }));
}

void InputDevicesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::InputDevices();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const DeviceInfo& value) { return DeviceToObject(isolate, context, value); }));
}

void OutputDevicesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    const auto values = engine::bridge::ffmpeg::OutputDevices();
    args.GetReturnValue().Set(VectorToArray(isolate, context, values,
        [=](const DeviceInfo& value) { return DeviceToObject(isolate, context, value); }));
}

void ProbeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    std::string path;
    if (!ReadStringArg(args, 0, &path)) {
        ThrowError(isolate, "FFmpeg.probe(path) expects a string path");
        return;
    }
    MediaInfo info;
    std::string error;
    if (!engine::bridge::ffmpeg::ProbeMedia(path, &info, &error)) {
        ThrowError(isolate, error.empty() ? "FFmpeg.probe failed" : error);
        return;
    }
    args.GetReturnValue().Set(MediaToObject(isolate, context, info));
}

void ReadPacketsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    std::string path;
    if (!ReadStringArg(args, 0, &path)) {
        ThrowError(isolate, "FFmpeg.readPackets(path, maxPackets?) expects a string path");
        return;
    }
    int max_packets = 128;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        max_packets = args[1]->Int32Value(context).FromMaybe(128);
    }
    std::vector<PacketInfo> packets;
    std::string error;
    if (!engine::bridge::ffmpeg::ReadPackets(path, max_packets, &packets, &error)) {
        ThrowError(isolate, error.empty() ? "FFmpeg.readPackets failed" : error);
        return;
    }
    args.GetReturnValue().Set(VectorToArray(isolate, context, packets,
        [=](const PacketInfo& value) { return PacketToObject(isolate, context, value); }));
}

void DecodeVideoFramesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    std::string path;
    if (!ReadStringArg(args, 0, &path)) {
        ThrowError(isolate, "FFmpeg.decodeVideoFrames(path, maxFrames?, streamIndex?) expects a string path");
        return;
    }
    int max_frames = 1;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        max_frames = args[1]->Int32Value(context).FromMaybe(1);
    }
    int stream_index = -1;
    if (args.Length() > 2 && args[2]->IsNumber()) {
        stream_index = args[2]->Int32Value(context).FromMaybe(-1);
    }
    std::vector<VideoFrameInfo> frames;
    std::string error;
    if (!engine::bridge::ffmpeg::DecodeVideoFrames(path, stream_index, max_frames, &frames, &error)) {
        ThrowError(isolate, error.empty() ? "FFmpeg.decodeVideoFrames failed" : error);
        return;
    }
    args.GetReturnValue().Set(VectorToArray(isolate, context, frames,
        [=](const VideoFrameInfo& value) { return VideoFrameToObject(isolate, context, value); }));
}

void DecodeAudioFramesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    std::string path;
    if (!ReadStringArg(args, 0, &path)) {
        ThrowError(isolate, "FFmpeg.decodeAudioFrames(path, maxFrames?, streamIndex?) expects a string path");
        return;
    }
    int max_frames = 1;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        max_frames = args[1]->Int32Value(context).FromMaybe(1);
    }
    int stream_index = -1;
    if (args.Length() > 2 && args[2]->IsNumber()) {
        stream_index = args[2]->Int32Value(context).FromMaybe(-1);
    }
    std::vector<AudioFrameInfo> frames;
    std::string error;
    if (!engine::bridge::ffmpeg::DecodeAudioFrames(path, stream_index, max_frames, &frames, &error)) {
        ThrowError(isolate, error.empty() ? "FFmpeg.decodeAudioFrames failed" : error);
        return;
    }
    args.GetReturnValue().Set(VectorToArray(isolate, context, frames,
        [=](const AudioFrameInfo& value) { return AudioFrameToObject(isolate, context, value); }));
}

void RemuxCopyCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::string input_path;
    std::string output_path;
    if (!ReadStringArg(args, 0, &input_path) || !ReadStringArg(args, 1, &output_path)) {
        ThrowError(args.GetIsolate(), "FFmpeg.remuxCopy(inputPath, outputPath) expects two string paths");
        return;
    }
    std::string error;
    if (!engine::bridge::ffmpeg::RemuxCopy(input_path, output_path, &error)) {
        ThrowError(args.GetIsolate(), error.empty() ? "FFmpeg.remuxCopy failed" : error);
        return;
    }
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void ExtractStreamCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    std::string input_path;
    std::string output_path;
    if (!ReadStringArg(args, 0, &input_path) ||
        args.Length() < 2 || !args[1]->IsNumber() ||
        !ReadStringArg(args, 2, &output_path)) {
        ThrowError(isolate, "FFmpeg.extractStream(inputPath, streamIndex, outputPath) expects string, number, string");
        return;
    }
    const int stream_index = args[1]->Int32Value(context).FromMaybe(-1);
    std::string error;
    if (!engine::bridge::ffmpeg::ExtractStream(input_path, stream_index, output_path, &error)) {
        ThrowError(isolate, error.empty() ? "FFmpeg.extractStream failed" : error);
        return;
    }
    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

}  // namespace

namespace modules {

bool RegisterFFmpegModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> module = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && Set(context, module, "available",
                   v8::Boolean::New(isolate, engine::bridge::ffmpeg::IsAvailable()));
    ok = ok && Set(context, module, "hasFilterLibrary",
                   v8::Boolean::New(isolate, engine::bridge::ffmpeg::HasFilterLibrary()));
    ok = ok && Set(context, module, "hasDeviceLibrary",
                   v8::Boolean::New(isolate, engine::bridge::ffmpeg::HasDeviceLibrary()));
    ok = ok && Set(context, module, "summary",
                   ToString(isolate, engine::bridge::ffmpeg::Summary()));
    ok = ok && Set(context, module, "versions",
                   v8::Function::New(context, VersionsCallback).ToLocalChecked());
    ok = ok && Set(context, module, "configuration",
                   v8::Function::New(context, ConfigurationCallback).ToLocalChecked());
    ok = ok && Set(context, module, "license",
                   v8::Function::New(context, LicenseCallback).ToLocalChecked());
    ok = ok && Set(context, module, "protocols",
                   v8::Function::New(context, ProtocolsCallback).ToLocalChecked());
    ok = ok && Set(context, module, "inputFormats",
                   v8::Function::New(context, InputFormatsCallback).ToLocalChecked());
    ok = ok && Set(context, module, "outputFormats",
                   v8::Function::New(context, OutputFormatsCallback).ToLocalChecked());
    ok = ok && Set(context, module, "codecs",
                   v8::Function::New(context, CodecsCallback).ToLocalChecked());
    ok = ok && Set(context, module, "encoders",
                   v8::Function::New(context, EncodersCallback).ToLocalChecked());
    ok = ok && Set(context, module, "decoders",
                   v8::Function::New(context, DecodersCallback).ToLocalChecked());
    ok = ok && Set(context, module, "bitstreamFilters",
                   v8::Function::New(context, BitstreamFiltersCallback).ToLocalChecked());
    ok = ok && Set(context, module, "filters",
                   v8::Function::New(context, FiltersCallback).ToLocalChecked());
    ok = ok && Set(context, module, "inputDevices",
                   v8::Function::New(context, InputDevicesCallback).ToLocalChecked());
    ok = ok && Set(context, module, "outputDevices",
                   v8::Function::New(context, OutputDevicesCallback).ToLocalChecked());
    ok = ok && Set(context, module, "probe",
                   v8::Function::New(context, ProbeCallback).ToLocalChecked());
    ok = ok && Set(context, module, "readPackets",
                   v8::Function::New(context, ReadPacketsCallback).ToLocalChecked());
    ok = ok && Set(context, module, "decodeVideoFrames",
                   v8::Function::New(context, DecodeVideoFramesCallback).ToLocalChecked());
    ok = ok && Set(context, module, "decodeAudioFrames",
                   v8::Function::New(context, DecodeAudioFramesCallback).ToLocalChecked());
    ok = ok && Set(context, module, "remuxCopy",
                   v8::Function::New(context, RemuxCopyCallback).ToLocalChecked());
    ok = ok && Set(context, module, "extractStream",
                   v8::Function::New(context, ExtractStreamCallback).ToLocalChecked());

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "FFmpeg"), module)
        .FromMaybe(false);
}

}  // namespace modules