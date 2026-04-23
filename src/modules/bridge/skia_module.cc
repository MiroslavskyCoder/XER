#include "modules/bridge/skia_module.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "wrapper/skia/skia_engine_bridge.h"

namespace {

double ReadNumber(const v8::FunctionCallbackInfo<v8::Value>& args, int index, double fallback = 0.0) {
    if (index >= args.Length() || !args[index]->IsNumber()) {
        return fallback;
    }
    return args[index].As<v8::Number>()->Value();
}

int ReadInt(const v8::FunctionCallbackInfo<v8::Value>& args, int index, int fallback = 0) {
    if (index >= args.Length() || !args[index]->IsNumber()) {
        return fallback;
    }
    return args[index].As<v8::Number>()->Value();
}

double ReadDbl(const v8::FunctionCallbackInfo<v8::Value>& args, int index, double fallback = 0.0) {
    if (index >= args.Length() || !args[index]->IsNumber()) {
        return fallback;
    }
    return args[index].As<v8::Number>()->Value();
}

bool ReadBool(const v8::FunctionCallbackInfo<v8::Value>& args, int index, bool fallback = false) {
    if (index >= args.Length()) {
        return fallback;
    }
    if (args[index]->IsBoolean()) {
        return args[index].As<v8::Boolean>()->Value();
    }
    if (args[index]->IsNumber()) {
        return args[index].As<v8::Number>()->Value() != 0.0;
    }
    return fallback;
}

uint32_t ReadU32(const v8::FunctionCallbackInfo<v8::Value>& args, int index, uint32_t fallback = 0) {
    if (index >= args.Length() || !args[index]->IsNumber()) {
        return fallback;
    }
    const double v = args[index].As<v8::Number>()->Value();
    if (v < 0) {
        return 0;
    }
    if (v > 4294967295.0) {
        return 0xFFFFFFFFu;
    }
    return static_cast<uint32_t>(v);
}

std::string ReadString(v8::Isolate* isolate,
                       const v8::FunctionCallbackInfo<v8::Value>& args,
                       int index,
                       const std::string& fallback = std::string()) {
    if (index >= args.Length() || !args[index]->IsString()) {
        return fallback;
    }
    v8::String::Utf8Value utf8(isolate, args[index]);
    if (*utf8 == nullptr) {
        return fallback;
    }
    return *utf8;
}

v8::Local<v8::Array> MatrixToArray(v8::Isolate* isolate,
                                   v8::Local<v8::Context> context,
                                   const engine::bridge::skia::Matrix3x3& m) {
    v8::Local<v8::Array> out = v8::Array::New(isolate, 9);
    for (int i = 0; i < 9; ++i) {
        (void)out->Set(context, i, v8::Number::New(isolate, m[static_cast<size_t>(i)])).FromMaybe(false);
    }
    return out;
}

bool MatrixFromValue(v8::Isolate* isolate,
                     v8::Local<v8::Context> context,
                     v8::Local<v8::Value> value,
                     engine::bridge::skia::Matrix3x3* out) {
    if (out == nullptr || !value->IsArray()) {
        return false;
    }
    v8::Local<v8::Array> arr = value.As<v8::Array>();
    if (arr->Length() != 9) {
        return false;
    }
    for (uint32_t i = 0; i < 9; ++i) {
        v8::Local<v8::Value> item;
        if (!arr->Get(context, i).ToLocal(&item) || !item->IsNumber()) {
            return false;
        }
        (*out)[static_cast<size_t>(i)] = item.As<v8::Number>()->Value();
    }
    return true;
}

bool ReadIntProperty(v8::Local<v8::Context> context,
                     v8::Local<v8::Object> object,
                     const char* key,
                     int* out) {
    if (out == nullptr) {
        return false;
    }
    v8::Local<v8::String> key_str =
        v8::String::NewFromUtf8(context->GetIsolate(), key).ToLocalChecked();
    v8::Local<v8::Value> value;
    if (!object->Get(context, key_str).ToLocal(&value)
        || !value->IsNumber()) {
        return false;
    }
    *out = value.As<v8::Number>()->Value();
    return true;
}

bool ReadArrayProperty(v8::Local<v8::Context> context,
                       v8::Local<v8::Object> object,
                       const char* key,
                       v8::Local<v8::Array>* out) {
    if (out == nullptr) {
        return false;
    }
    v8::Local<v8::String> key_str =
        v8::String::NewFromUtf8(context->GetIsolate(), key).ToLocalChecked();
    v8::Local<v8::Value> value;
    if (!object->Get(context, key_str).ToLocal(&value)
        || !value->IsArray()) {
        return false;
    }
    *out = value.As<v8::Array>();
    return true;
}

bool ReadBoolProperty(v8::Local<v8::Context> context,
                      v8::Local<v8::Object> object,
                      const char* key,
                      bool* out) {
    if (out == nullptr) {
        return false;
    }
    v8::Local<v8::String> key_str =
        v8::String::NewFromUtf8(context->GetIsolate(), key).ToLocalChecked();
    v8::Local<v8::Value> value;
    if (!object->Get(context, key_str).ToLocal(&value) || !value->IsBoolean()) {
        return false;
    }
    *out = value.As<v8::Boolean>()->Value();
    return true;
}

bool ReadNumberProperty(v8::Local<v8::Context> context,
                        v8::Local<v8::Object> object,
                        const char* key,
                        double* out) {
    if (out == nullptr) {
        return false;
    }
    v8::Local<v8::String> key_str =
        v8::String::NewFromUtf8(context->GetIsolate(), key).ToLocalChecked();
    v8::Local<v8::Value> value;
    if (!object->Get(context, key_str).ToLocal(&value) || !value->IsNumber()) {
        return false;
    }
    *out = value.As<v8::Number>()->Value();
    return true;
}

bool ReadStringProperty(v8::Isolate* isolate,
                        v8::Local<v8::Context> context,
                        v8::Local<v8::Object> object,
                        const char* key,
                        std::string* out) {
    if (out == nullptr) {
        return false;
    }
    v8::Local<v8::String> key_str =
        v8::String::NewFromUtf8(context->GetIsolate(), key).ToLocalChecked();
    v8::Local<v8::Value> value;
    if (!object->Get(context, key_str).ToLocal(&value) || !value->IsString()) {
        return false;
    }
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return false;
    }
    *out = *utf8;
    return true;
}

bool ReadCanvasData(v8::Local<v8::Context> context,
                    v8::Local<v8::Object> object,
                    int* width,
                    int* height,
                    v8::Local<v8::Array>* pixels) {
    return ReadIntProperty(context, object, "width", width)
        && ReadIntProperty(context, object, "height", height)
        && ReadArrayProperty(context, object, "_pixels", pixels);
}

bool GetSkiaConstructor(v8::Local<v8::Context> context,
                        const char* class_name,
                        v8::Local<v8::Function>* ctor_out) {
    if (ctor_out == nullptr) {
        return false;
    }
    v8::Local<v8::Value> skia_value;
    if (!context->Global()->Get(context, v8::String::NewFromUtf8Literal(context->GetIsolate(), "Skia")).ToLocal(&skia_value)
        || !skia_value->IsObject()) {
        return false;
    }
    v8::Local<v8::Object> skia_obj = skia_value.As<v8::Object>();
    v8::Local<v8::String> class_name_str =
        v8::String::NewFromUtf8(context->GetIsolate(), class_name).ToLocalChecked();
    v8::Local<v8::Value> ctor_value;
    if (!skia_obj->Get(context, class_name_str).ToLocal(&ctor_value)
        || !ctor_value->IsFunction()) {
        return false;
    }
    *ctor_out = ctor_value.As<v8::Function>();
    return true;
}

void InitPixelArray(v8::Isolate* isolate,
                    v8::Local<v8::Context> context,
                    v8::Local<v8::Array> pixels,
                    uint32_t color) {
    for (uint32_t i = 0; i < pixels->Length(); ++i) {
        (void)pixels->Set(context, i, v8::Number::New(isolate, static_cast<double>(color))).FromMaybe(false);
    }
}

void CopyPixelArray(v8::Isolate* isolate,
                    v8::Local<v8::Context> context,
                    v8::Local<v8::Array> src,
                    v8::Local<v8::Array> dst) {
    const uint32_t count = std::min(src->Length(), dst->Length());
    for (uint32_t i = 0; i < count; ++i) {
        v8::Local<v8::Value> value;
        if (!src->Get(context, i).ToLocal(&value) || !value->IsNumber()) {
            value = v8::Number::New(isolate, 0);
        }
        (void)dst->Set(context, i, value).FromMaybe(false);
    }
}

bool PixelArrayToVector(v8::Local<v8::Context> context,
                        v8::Local<v8::Array> pixels,
                        int width,
                        int height,
                        std::vector<uint32_t>* out) {
    if (out == nullptr || width <= 0 || height <= 0) {
        return false;
    }
    const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (pixels->Length() < expected) {
        return false;
    }
    out->assign(expected, 0);
    for (size_t i = 0; i < expected; ++i) {
        v8::Local<v8::Value> value;
        if (!pixels->Get(context, static_cast<uint32_t>(i)).ToLocal(&value) || !value->IsNumber()) {
            continue;
        }
        const double raw = value.As<v8::Number>()->Value();
        if (raw < 0.0) {
            (*out)[i] = 0;
            continue;
        }
        if (raw > 4294967295.0) {
            (*out)[i] = 0xFFFFFFFFu;
            continue;
        }
        (*out)[i] = static_cast<uint32_t>(raw);
    }
    return true;
}

void PixelVectorToArray(v8::Isolate* isolate,
                        v8::Local<v8::Context> context,
                        const std::vector<uint32_t>& src,
                        v8::Local<v8::Array> dst) {
    const size_t count = std::min(src.size(), static_cast<size_t>(dst->Length()));
    for (size_t i = 0; i < count; ++i) {
        (void)dst->Set(context,
                       static_cast<uint32_t>(i),
                       v8::Number::New(isolate, static_cast<double>(src[i])))
            .FromMaybe(false);
    }
}

bool CopyCanvasData(v8::Local<v8::Context> context,
                    v8::Local<v8::Object> object,
                    int* width,
                    int* height,
                    v8::Local<v8::Array>* pixels_copy) {
    v8::Isolate* isolate = context->GetIsolate();
    int src_width = 0;
    int src_height = 0;
    v8::Local<v8::Array> src_pixels;
    if (!ReadCanvasData(context, object, &src_width, &src_height, &src_pixels)) {
        return false;
    }
    if (width != nullptr) {
        *width = src_width;
    }
    if (height != nullptr) {
        *height = src_height;
    }
    if (pixels_copy != nullptr) {
        v8::Local<v8::Array> copy = v8::Array::New(isolate, src_pixels->Length());
        CopyPixelArray(isolate, context, src_pixels, copy);
        *pixels_copy = copy;
    }
    return true;
}

int PixelIndex(int x, int y, int width, int height) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return -1;
    }
    return y * width + x;
}

struct PaintState {
    uint32_t color = 0;
    double alpha = 1.0;
    std::string blend_mode = "srcOver";
    bool fill = false;
};

uint8_t ColorR(uint32_t c) {
    return static_cast<uint8_t>((c >> 24) & 0xFF);
}

uint8_t ColorG(uint32_t c) {
    return static_cast<uint8_t>((c >> 16) & 0xFF);
}

uint8_t ColorB(uint32_t c) {
    return static_cast<uint8_t>((c >> 8) & 0xFF);
}

uint8_t ColorA(uint32_t c) {
    return static_cast<uint8_t>(c & 0xFF);
}

uint32_t ComposeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (static_cast<uint32_t>(r) << 24)
        | (static_cast<uint32_t>(g) << 16)
        | (static_cast<uint32_t>(b) << 8)
        | static_cast<uint32_t>(a);
}

uint32_t ApplyAlpha(uint32_t color, double alpha) {
    if (alpha < 0.0) {
        alpha = 0.0;
    }
    if (alpha > 1.0) {
        alpha = 1.0;
    }
    const int base_a = static_cast<int>(ColorA(color));
    const int out_a = static_cast<int>(std::round(base_a * alpha));
    return ComposeColor(ColorR(color), ColorG(color), ColorB(color), static_cast<uint8_t>(std::clamp(out_a, 0, 255)));
}

PaintState ReadPaintState(v8::Isolate* isolate,
                          v8::Local<v8::Context> context,
                          v8::Local<v8::Value> value,
                          uint32_t default_color,
                          bool default_fill) {
    PaintState paint;
    paint.color = default_color;
    paint.fill = default_fill;

    if (value.IsEmpty() || value->IsUndefined() || value->IsNull()) {
        return paint;
    }

    if (value->IsNumber()) {
        const double raw = value.As<v8::Number>()->Value();
        paint.color = raw < 0.0 ? 0u : static_cast<uint32_t>(raw);
        return paint;
    }

    if (!value->IsObject()) {
        return paint;
    }

    v8::Local<v8::Object> object = value.As<v8::Object>();
    int color_prop = 0;
    if (ReadIntProperty(context, object, "color", &color_prop)) {
        paint.color = color_prop < 0 ? 0u : static_cast<uint32_t>(color_prop);
    }

    (void)ReadNumberProperty(context, object, "alpha", &paint.alpha);
    (void)ReadBoolProperty(context, object, "fill", &paint.fill);
    (void)ReadStringProperty(isolate, context, object, "blendMode", &paint.blend_mode);

    paint.color = ApplyAlpha(paint.color, paint.alpha);
    return paint;
}

bool ReadPixelColor(v8::Local<v8::Context> context,
                    v8::Local<v8::Array> pixels,
                    int idx,
                    uint32_t* out_color) {
    if (out_color == nullptr || idx < 0) {
        return false;
    }
    v8::Local<v8::Value> value;
    if (!pixels->Get(context, static_cast<uint32_t>(idx)).ToLocal(&value) || !value->IsNumber()) {
        return false;
    }
    const double raw = value.As<v8::Number>()->Value();
    if (raw < 0.0) {
        *out_color = 0;
        return true;
    }
    *out_color = static_cast<uint32_t>(raw);
    return true;
}

bool SetPixelAt(v8::Isolate* isolate,
                v8::Local<v8::Context> context,
                v8::Local<v8::Array> pixels,
                int width,
                int height,
                int x,
                int y,
                uint32_t color) {
    const int idx = PixelIndex(x, y, width, height);
    if (idx < 0) {
        return false;
    }
    return pixels
        ->Set(context,
              static_cast<uint32_t>(idx),
              v8::Number::New(isolate, static_cast<double>(color)))
        .FromMaybe(false);
}

bool BlendPixelAt(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  v8::Local<v8::Array> pixels,
                  int width,
                  int height,
                  int x,
                  int y,
                  const PaintState& paint) {
    const int idx = PixelIndex(x, y, width, height);
    if (idx < 0) {
        return false;
    }
    uint32_t dst_color = 0;
    (void)ReadPixelColor(context, pixels, idx, &dst_color);
    const uint32_t out = engine::bridge::skia::BlendModeApply(paint.blend_mode, dst_color, paint.color);
    return pixels
        ->Set(context,
              static_cast<uint32_t>(idx),
              v8::Number::New(isolate, static_cast<double>(out)))
        .FromMaybe(false);
}

bool ParsePointsArray(v8::Isolate* isolate,
                      v8::Local<v8::Context> context,
                      v8::Local<v8::Value> value,
                      std::vector<std::pair<int, int>>* out_points) {
    if (out_points == nullptr || !value->IsArray()) {
        return false;
    }

    out_points->clear();
    v8::Local<v8::Array> arr = value.As<v8::Array>();
    if (arr->Length() == 0) {
        return true;
    }

    v8::Local<v8::Value> first;
    if (!arr->Get(context, 0).ToLocal(&first)) {
        return false;
    }

    if (first->IsArray()) {
        for (uint32_t i = 0; i < arr->Length(); ++i) {
            v8::Local<v8::Value> pair_val;
            if (!arr->Get(context, i).ToLocal(&pair_val) || !pair_val->IsArray()) {
                return false;
            }
            v8::Local<v8::Array> pair_arr = pair_val.As<v8::Array>();
            if (pair_arr->Length() < 2) {
                return false;
            }
            v8::Local<v8::Value> vx;
            v8::Local<v8::Value> vy;
            if (!pair_arr->Get(context, 0).ToLocal(&vx) || !pair_arr->Get(context, 1).ToLocal(&vy)
                || !vx->IsNumber() || !vy->IsNumber()) {
                return false;
            }
            out_points->push_back({vx.As<v8::Number>()->Value(), vy.As<v8::Number>()->Value()});
        }
        return true;
    }

    if (arr->Length() % 2 != 0) {
        return false;
    }
    for (uint32_t i = 0; i < arr->Length(); i += 2) {
        v8::Local<v8::Value> vx;
        v8::Local<v8::Value> vy;
        if (!arr->Get(context, i).ToLocal(&vx) || !arr->Get(context, i + 1).ToLocal(&vy)
            || !vx->IsNumber() || !vy->IsNumber()) {
            return false;
        }
        out_points->push_back({vx.As<v8::Number>()->Value(), vy.As<v8::Number>()->Value()});
    }
    return true;
}

void DrawLineWithPaint(v8::Isolate* isolate,
                       v8::Local<v8::Context> context,
                       v8::Local<v8::Array> pixels,
                       int width,
                       int height,
                       int x0,
                       int y0,
                       int x1,
                       int y1,
                       const PaintState& paint) {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        (void)BlendPixelAt(isolate, context, pixels, width, height, x0, y0, paint);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void FillPolygonWithPaint(v8::Isolate* isolate,
                          v8::Local<v8::Context> context,
                          v8::Local<v8::Array> pixels,
                          int width,
                          int height,
                          const std::vector<std::pair<int, int>>& points,
                          const PaintState& paint) {
    if (points.size() < 3) {
        return;
    }

    int min_y = points[0].second;
    int max_y = points[0].second;
    for (const auto& p : points) {
        min_y = std::min(min_y, p.second);
        max_y = std::max(max_y, p.second);
    }

    min_y = std::max(0, min_y);
    max_y = std::min(height - 1, max_y);

    for (int y = min_y; y <= max_y; ++y) {
        std::vector<int> intersections;
        for (size_t i = 0; i < points.size(); ++i) {
            const auto& p1 = points[i];
            const auto& p2 = points[(i + 1) % points.size()];
            const int y1 = p1.second;
            const int y2 = p2.second;
            if ((y1 <= y && y < y2) || (y2 <= y && y < y1)) {
                const double t = static_cast<double>(y - y1) / static_cast<double>(y2 - y1);
                const double xf = p1.first + t * static_cast<double>(p2.first - p1.first);
                intersections.push_back(static_cast<int>(std::round(xf)));
            }
        }

        std::sort(intersections.begin(), intersections.end());
        for (size_t k = 0; k + 1 < intersections.size(); k += 2) {
            int x_start = intersections[k];
            int x_end = intersections[k + 1];
            if (x_start > x_end) {
                std::swap(x_start, x_end);
            }
            x_start = std::max(0, x_start);
            x_end = std::min(width - 1, x_end);
            for (int x = x_start; x <= x_end; ++x) {
                (void)BlendPixelAt(isolate, context, pixels, width, height, x, y, paint);
            }
        }
    }
}

void CanvasWidthCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
    int width = 0;
    if (ReadIntProperty(context, args.This(), "width", &width)) {
        args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), width));
        return;
    }
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
}

void CanvasHeightCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
    int height = 0;
    if (ReadIntProperty(context, args.This(), "height", &height)) {
        args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), height));
        return;
    }
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
}

void CanvasConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (!args.IsConstructCall()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Skia.Canvas must be called with new")));
        return;
    }

    int width = ReadInt(args, 0, 1);
    int height = ReadInt(args, 1, 1);
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    if (width > 8192) width = 8192;
    if (height > 8192) height = 8192;

    const uint32_t fill = ReadU32(args, 2, 0);
    const uint32_t count = static_cast<uint32_t>(width * height);
    v8::Local<v8::Array> pixels = v8::Array::New(isolate, count);
    InitPixelArray(isolate, context, pixels, fill);

    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "width"), v8::Integer::New(isolate, width)).FromMaybe(false);
    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "height"), v8::Integer::New(isolate, height)).FromMaybe(false);
    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "_pixels"), pixels).FromMaybe(false);
}

void ImageConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (!args.IsConstructCall()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Skia.Image must be called with new")));
        return;
    }

    int width = ReadInt(args, 0, 1);
    int height = ReadInt(args, 1, 1);
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    if (width > 8192) width = 8192;
    if (height > 8192) height = 8192;

    const uint32_t count = static_cast<uint32_t>(width * height);
    v8::Local<v8::Array> pixels = v8::Array::New(isolate, count);

    if (args.Length() > 2 && args[2]->IsArray()) {
        CopyPixelArray(isolate, context, args[2].As<v8::Array>(), pixels);
    } else {
        const uint32_t fill = ReadU32(args, 2, 0);
        InitPixelArray(isolate, context, pixels, fill);
    }

    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "width"), v8::Integer::New(isolate, width)).FromMaybe(false);
    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "height"), v8::Integer::New(isolate, height)).FromMaybe(false);
    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "_pixels"), pixels).FromMaybe(false);
}

void SurfaceConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (!args.IsConstructCall()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Skia.Surface must be called with new")));
        return;
    }

    v8::Local<v8::Function> canvas_ctor;
    if (!GetSkiaConstructor(context, "Canvas", &canvas_ctor)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Skia.Canvas constructor is unavailable")));
        return;
    }

    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, ReadInt(args, 0, 1)),
        v8::Integer::New(isolate, ReadInt(args, 1, 1)),
        v8::Number::New(isolate, static_cast<double>(ReadU32(args, 2, 0)))
    };

    v8::Local<v8::Object> canvas;
    if (!canvas_ctor->NewInstance(context, 3, ctor_args).ToLocal(&canvas)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to create Canvas for Surface")));
        return;
    }

    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "_canvas"), canvas).FromMaybe(false);
}

void CanvasGetPixelCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Number::New(isolate, 0));
        return;
    }
    const int idx = PixelIndex(ReadInt(args, 0), ReadInt(args, 1), width, height);
    if (idx < 0) {
        args.GetReturnValue().Set(v8::Number::New(isolate, 0));
        return;
    }
    v8::Local<v8::Value> value;
    if (!pixels->Get(context, static_cast<uint32_t>(idx)).ToLocal(&value) || !value->IsNumber()) {
        args.GetReturnValue().Set(v8::Number::New(isolate, 0));
        return;
    }
    args.GetReturnValue().Set(value);
}

void CanvasSetPixelCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
        return;
    }
    const int idx = PixelIndex(ReadInt(args, 0), ReadInt(args, 1), width, height);
    if (idx < 0) {
        args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
        return;
    }
    const bool ok = pixels->Set(context,
                                static_cast<uint32_t>(idx),
                                v8::Number::New(isolate, static_cast<double>(ReadU32(args, 2, 0))))
                        .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void CanvasClearCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }
    const uint32_t color = ReadU32(args, 0, 0);
    if (engine::bridge::skia::HasNativeRaster()) {
        std::vector<uint32_t> native_pixels;
        if (PixelArrayToVector(context, pixels, width, height, &native_pixels)
            && engine::bridge::skia::RasterClear(&native_pixels, width, height, color)) {
            PixelVectorToArray(isolate, context, native_pixels, pixels);
            return;
        }
    }
    InitPixelArray(isolate, context, pixels, color);
}

void CanvasDrawRectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }

    const int x = ReadInt(args, 0, 0);
    const int y = ReadInt(args, 1, 0);
    const int w = ReadInt(args, 2, 0);
    const int h = ReadInt(args, 3, 0);
    const PaintState paint = ReadPaintState(
        isolate,
        context,
        args.Length() > 5 ? args[5] : v8::Local<v8::Value>(),
        ReadU32(args, 4, 0),
        true);

    if (engine::bridge::skia::HasNativeRaster()) {
        std::vector<uint32_t> native_pixels;
        if (PixelArrayToVector(context, pixels, width, height, &native_pixels)
            && engine::bridge::skia::RasterDrawRect(&native_pixels,
                                                    width,
                                                    height,
                                                    x,
                                                    y,
                                                    w,
                                                    h,
                                                    paint.color,
                                                    paint.blend_mode)) {
            PixelVectorToArray(isolate, context, native_pixels, pixels);
            return;
        }
    }

    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(width, x + std::max(0, w));
    const int y1 = std::min(height, y + std::max(0, h));
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const int idx = py * width + px;
            uint32_t dst = 0;
            (void)ReadPixelColor(context, pixels, idx, &dst);
            const uint32_t out = engine::bridge::skia::BlendModeApply(paint.blend_mode, dst, paint.color);
            (void)pixels->Set(context, static_cast<uint32_t>(idx), v8::Number::New(isolate, static_cast<double>(out))).FromMaybe(false);
        }
    }
}

void CanvasDrawLineCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }

    int x0 = ReadInt(args, 0, 0);
    int y0 = ReadInt(args, 1, 0);
    const int x1 = ReadInt(args, 2, 0);
    const int y1 = ReadInt(args, 3, 0);
    const PaintState paint = ReadPaintState(
        isolate,
        context,
        args.Length() > 5 ? args[5] : v8::Local<v8::Value>(),
        ReadU32(args, 4, 0),
        false);

    if (engine::bridge::skia::HasNativeRaster()) {
        std::vector<uint32_t> native_pixels;
        if (PixelArrayToVector(context, pixels, width, height, &native_pixels)
            && engine::bridge::skia::RasterDrawLine(&native_pixels,
                                                    width,
                                                    height,
                                                    x0,
                                                    y0,
                                                    x1,
                                                    y1,
                                                    paint.color,
                                                    paint.blend_mode)) {
            PixelVectorToArray(isolate, context, native_pixels, pixels);
            return;
        }
    }

    DrawLineWithPaint(isolate, context, pixels, width, height, x0, y0, x1, y1, paint);
}

void CanvasDrawCircleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }

    const int cx = ReadInt(args, 0, 0);
    const int cy = ReadInt(args, 1, 0);
    int r = ReadInt(args, 2, 0);
    if (r < 0) {
        r = -r;
    }
    const PaintState paint = ReadPaintState(
        isolate,
        context,
        args.Length() > 5 ? args[5] : v8::Local<v8::Value>(),
        ReadU32(args, 3, 0),
        (args.Length() > 4 && args[4]->IsBoolean())
            ? args[4].As<v8::Boolean>()->Value()
            : false);

    if (engine::bridge::skia::HasNativeRaster()) {
        std::vector<uint32_t> native_pixels;
        if (PixelArrayToVector(context, pixels, width, height, &native_pixels)
            && engine::bridge::skia::RasterDrawCircle(&native_pixels,
                                                      width,
                                                      height,
                                                      cx,
                                                      cy,
                                                      r,
                                                      paint.color,
                                                      paint.blend_mode,
                                                      paint.fill)) {
            PixelVectorToArray(isolate, context, native_pixels, pixels);
            return;
        }
    }

    if (r == 0) {
        (void)BlendPixelAt(isolate, context, pixels, width, height, cx, cy, paint);
        return;
    }

    int x = r;
    int y = 0;
    int err = 1 - x;

    auto draw_hline = [&](int x_start, int x_end, int py) {
        if (py < 0 || py >= height) {
            return;
        }
        const int xs = std::max(0, x_start);
        const int xe = std::min(width - 1, x_end);
        for (int px = xs; px <= xe; ++px) {
            (void)BlendPixelAt(isolate, context, pixels, width, height, px, py, paint);
        }
    };

    while (x >= y) {
        if (paint.fill) {
            draw_hline(cx - x, cx + x, cy + y);
            draw_hline(cx - y, cx + y, cy + x);
            draw_hline(cx - x, cx + x, cy - y);
            draw_hline(cx - y, cx + y, cy - x);
        } else {
            (void)BlendPixelAt(isolate, context, pixels, width, height, cx + x, cy + y, paint);
            (void)BlendPixelAt(isolate, context, pixels, width, height, cx + y, cy + x, paint);
            (void)BlendPixelAt(isolate, context, pixels, width, height, cx - y, cy + x, paint);
            (void)BlendPixelAt(isolate, context, pixels, width, height, cx - x, cy + y, paint);
            (void)BlendPixelAt(isolate, context, pixels, width, height, cx - x, cy - y, paint);
            (void)BlendPixelAt(isolate, context, pixels, width, height, cx - y, cy - x, paint);
            (void)BlendPixelAt(isolate, context, pixels, width, height, cx + y, cy - x, paint);
            (void)BlendPixelAt(isolate, context, pixels, width, height, cx + x, cy - y, paint);
        }

        ++y;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            --x;
            err += 2 * (y - x + 1);
        }
    }
}

void CanvasDrawTriangleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }

    const int x1 = ReadInt(args, 0, 0);
    const int y1 = ReadInt(args, 1, 0);
    const int x2 = ReadInt(args, 2, 0);
    const int y2 = ReadInt(args, 3, 0);
    const int x3 = ReadInt(args, 4, 0);
    const int y3 = ReadInt(args, 5, 0);

    const PaintState paint = ReadPaintState(
        isolate,
        context,
        args.Length() > 8 ? args[8] : v8::Local<v8::Value>(),
        ReadU32(args, 6, 0),
        (args.Length() > 7 && args[7]->IsBoolean())
            ? args[7].As<v8::Boolean>()->Value()
            : false);

    if (engine::bridge::skia::HasNativeRaster()) {
        std::vector<uint32_t> native_pixels;
        if (PixelArrayToVector(context, pixels, width, height, &native_pixels)
            && engine::bridge::skia::RasterDrawTriangle(&native_pixels,
                                                        width,
                                                        height,
                                                        x1,
                                                        y1,
                                                        x2,
                                                        y2,
                                                        x3,
                                                        y3,
                                                        paint.color,
                                                        paint.blend_mode,
                                                        paint.fill)) {
            PixelVectorToArray(isolate, context, native_pixels, pixels);
            return;
        }
    }

    if (paint.fill) {
        const std::vector<std::pair<int, int>> pts = {{x1, y1}, {x2, y2}, {x3, y3}};
        FillPolygonWithPaint(isolate, context, pixels, width, height, pts, paint);
        return;
    }

    DrawLineWithPaint(isolate, context, pixels, width, height, x1, y1, x2, y2, paint);
    DrawLineWithPaint(isolate, context, pixels, width, height, x2, y2, x3, y3, paint);
    DrawLineWithPaint(isolate, context, pixels, width, height, x3, y3, x1, y1, paint);
}

void CanvasDrawPolylineCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }

    std::vector<std::pair<int, int>> points;
    if (args.Length() < 1 || !ParsePointsArray(isolate, context, args[0], &points) || points.size() < 2) {
        return;
    }

    const bool closed = (args.Length() > 2 && args[2]->IsBoolean())
        ? args[2].As<v8::Boolean>()->Value()
        : false;

    const PaintState paint = ReadPaintState(
        isolate,
        context,
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(),
        ReadU32(args, 1, 0),
        false);

    if (engine::bridge::skia::HasNativeRaster()) {
        std::vector<uint32_t> native_pixels;
        if (PixelArrayToVector(context, pixels, width, height, &native_pixels)
            && engine::bridge::skia::RasterDrawPolyline(&native_pixels,
                                                        width,
                                                        height,
                                                        points,
                                                        paint.color,
                                                        paint.blend_mode,
                                                        closed)) {
            PixelVectorToArray(isolate, context, native_pixels, pixels);
            return;
        }
    }

    for (size_t i = 0; i + 1 < points.size(); ++i) {
        DrawLineWithPaint(isolate,
                          context,
                          pixels,
                          width,
                          height,
                          points[i].first,
                          points[i].second,
                          points[i + 1].first,
                          points[i + 1].second,
                          paint);
    }
    if (closed && points.size() > 2) {
        DrawLineWithPaint(isolate,
                          context,
                          pixels,
                          width,
                          height,
                          points.back().first,
                          points.back().second,
                          points.front().first,
                          points.front().second,
                          paint);
    }
}

void CanvasFillPolygonCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }

    std::vector<std::pair<int, int>> points;
    if (args.Length() < 1 || !ParsePointsArray(isolate, context, args[0], &points) || points.size() < 3) {
        return;
    }

    const PaintState paint = ReadPaintState(
        isolate,
        context,
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(),
        ReadU32(args, 1, 0),
        true);

    if (engine::bridge::skia::HasNativeRaster()) {
        std::vector<uint32_t> native_pixels;
        if (PixelArrayToVector(context, pixels, width, height, &native_pixels)
            && engine::bridge::skia::RasterFillPolygon(&native_pixels,
                                                       width,
                                                       height,
                                                       points,
                                                       paint.color,
                                                       paint.blend_mode)) {
            PixelVectorToArray(isolate, context, native_pixels, pixels);
            return;
        }
    }

    FillPolygonWithPaint(isolate, context, pixels, width, height, points, paint);
}

void PaintConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (!args.IsConstructCall()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Skia.Paint must be called with new")));
        return;
    }

    PaintState paint = ReadPaintState(
        isolate,
        context,
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(),
        ReadU32(args, 1, 0xFFFFFFFFu),
        false);

    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "color"),
                           v8::Number::New(isolate, static_cast<double>(paint.color)))
        .FromMaybe(false);
    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "alpha"),
                           v8::Number::New(isolate, paint.alpha))
        .FromMaybe(false);
    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "blendMode"),
                           v8::String::NewFromUtf8(isolate, paint.blend_mode.c_str()).ToLocalChecked())
        .FromMaybe(false);
    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "fill"),
                           v8::Boolean::New(isolate, paint.fill))
        .FromMaybe(false);
}

void CanvasDrawImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int dst_w = 0;
    int dst_h = 0;
    v8::Local<v8::Array> dst_pixels;
    if (!ReadCanvasData(context, args.This(), &dst_w, &dst_h, &dst_pixels)) {
        return;
    }

    if (args.Length() < 1 || !args[0]->IsObject()) {
        return;
    }

    v8::Local<v8::Object> src_obj = args[0].As<v8::Object>();
    int src_w = 0;
    int src_h = 0;
    v8::Local<v8::Array> src_pixels;
    if (!ReadCanvasData(context, src_obj, &src_w, &src_h, &src_pixels)) {
        return;
    }

    const int dx = ReadInt(args, 1, 0);
    const int dy = ReadInt(args, 2, 0);

    if (engine::bridge::skia::HasNativeRaster()) {
        std::vector<uint32_t> native_dst;
        std::vector<uint32_t> native_src;
        if (PixelArrayToVector(context, dst_pixels, dst_w, dst_h, &native_dst)
            && PixelArrayToVector(context, src_pixels, src_w, src_h, &native_src)
            && engine::bridge::skia::RasterDrawImage(&native_dst,
                                                     dst_w,
                                                     dst_h,
                                                     native_src,
                                                     src_w,
                                                     src_h,
                                                     dx,
                                                     dy,
                                                     "srcOver")) {
            PixelVectorToArray(isolate, context, native_dst, dst_pixels);
            return;
        }
    }

    for (int sy = 0; sy < src_h; ++sy) {
        for (int sx = 0; sx < src_w; ++sx) {
            const int tx = dx + sx;
            const int ty = dy + sy;
            const int t_idx = PixelIndex(tx, ty, dst_w, dst_h);
            if (t_idx < 0) {
                continue;
            }
            const int s_idx = sy * src_w + sx;
            v8::Local<v8::Value> pixel;
            if (!src_pixels->Get(context, static_cast<uint32_t>(s_idx)).ToLocal(&pixel) || !pixel->IsNumber()) {
                continue;
            }
            (void)dst_pixels->Set(context, static_cast<uint32_t>(t_idx), pixel).FromMaybe(false);
        }
    }
}

void CanvasPixelsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Array> pixels;
    if (!ReadArrayProperty(context, args.This(), "_pixels", &pixels)) {
        args.GetReturnValue().Set(v8::Array::New(isolate));
        return;
    }
    v8::Local<v8::Array> copy = v8::Array::New(isolate, pixels->Length());
    CopyPixelArray(isolate, context, pixels, copy);
    args.GetReturnValue().Set(copy);
}

void CanvasCloneCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> copy;
    if (!CopyCanvasData(context, args.This(), &width, &height, &copy)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Function> canvas_ctor;
    if (!GetSkiaConstructor(context, "Canvas", &canvas_ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, width),
        v8::Integer::New(isolate, height),
        v8::Number::New(isolate, 0)
    };
    v8::Local<v8::Object> canvas;
    if (!canvas_ctor->NewInstance(context, 3, ctor_args).ToLocal(&canvas)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Array> dst_pixels;
    if (!ReadArrayProperty(context, canvas, "_pixels", &dst_pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    CopyPixelArray(isolate, context, copy, dst_pixels);
    args.GetReturnValue().Set(canvas);
}

void CanvasResizeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    int new_width = ReadInt(args, 0, 1);
    int new_height = ReadInt(args, 1, 1);
    if (new_width < 1) new_width = 1;
    if (new_height < 1) new_height = 1;
    if (new_width > 8192) new_width = 8192;
    if (new_height > 8192) new_height = 8192;

    const uint32_t fill = ReadU32(args, 2, 0);
    const uint32_t new_count = static_cast<uint32_t>(new_width * new_height);
    v8::Local<v8::Array> new_pixels = v8::Array::New(isolate, new_count);
    InitPixelArray(isolate, context, new_pixels, fill);

    int old_width = 0;
    int old_height = 0;
    v8::Local<v8::Array> old_pixels;
    if (ReadCanvasData(context, args.This(), &old_width, &old_height, &old_pixels)) {
        const int copy_w = std::min(old_width, new_width);
        const int copy_h = std::min(old_height, new_height);
        for (int y = 0; y < copy_h; ++y) {
            for (int x = 0; x < copy_w; ++x) {
                const int old_idx = y * old_width + x;
                const int new_idx = y * new_width + x;
                v8::Local<v8::Value> pixel;
                if (old_pixels->Get(context, static_cast<uint32_t>(old_idx)).ToLocal(&pixel)) {
                    (void)new_pixels->Set(context, static_cast<uint32_t>(new_idx), pixel).FromMaybe(false);
                }
            }
        }
    }

    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "width"),
                           v8::Integer::New(isolate, new_width)).FromMaybe(false);
    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "height"),
                           v8::Integer::New(isolate, new_height)).FromMaybe(false);
    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "_pixels"),
                           new_pixels).FromMaybe(false);
}

void CanvasToSurfaceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Function> surface_ctor;
    if (!GetSkiaConstructor(context, "Surface", &surface_ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, width),
        v8::Integer::New(isolate, height),
        v8::Number::New(isolate, 0)
    };
    v8::Local<v8::Object> surface;
    if (!surface_ctor->NewInstance(context, 3, ctor_args).ToLocal(&surface)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Value> canvas_value;
    if (!surface->Get(context, v8::String::NewFromUtf8Literal(isolate, "_canvas")).ToLocal(&canvas_value)
        || !canvas_value->IsObject()) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Array> dst_pixels;
    if (!ReadArrayProperty(context, canvas_value.As<v8::Object>(), "_pixels", &dst_pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    CopyPixelArray(isolate, context, pixels, dst_pixels);
    args.GetReturnValue().Set(surface);
}

bool CallSurfaceCanvasMethod(v8::Isolate* isolate,
                             v8::Local<v8::Context> context,
                             v8::Local<v8::Object> surface,
                             const char* method_name,
                             int argc,
                             v8::Local<v8::Value>* argv,
                             v8::Local<v8::Value>* out) {
    v8::Local<v8::Value> canvas_value;
    if (!surface->Get(context, v8::String::NewFromUtf8Literal(isolate, "_canvas")).ToLocal(&canvas_value)
        || !canvas_value->IsObject()) {
        return false;
    }
    v8::Local<v8::Object> canvas = canvas_value.As<v8::Object>();
    v8::Local<v8::String> method_key = v8::String::NewFromUtf8(isolate, method_name).ToLocalChecked();
    v8::Local<v8::Value> method_value;
    if (!canvas->Get(context, method_key).ToLocal(&method_value) || !method_value->IsFunction()) {
        return false;
    }
    v8::Local<v8::Value> result;
    if (!method_value.As<v8::Function>()->Call(context, canvas, argc, argv).ToLocal(&result)) {
        return false;
    }
    if (out != nullptr) {
        *out = result;
    }
    return true;
}

void SurfaceClearCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[1] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Number::New(isolate, 0))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "clear", 1, argv, nullptr);
}

void SurfaceToImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> out;
    if (!CallSurfaceCanvasMethod(isolate, context, args.This(), "toImage", 0, nullptr, &out)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    args.GetReturnValue().Set(out);
}

void SurfaceDrawRectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[6] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 4 ? args[4] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 5 ? args[5] : v8::Local<v8::Value>(v8::Undefined(isolate))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "drawRect", 6, argv, nullptr);
}

void SurfaceDrawLineCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[6] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 4 ? args[4] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 5 ? args[5] : v8::Local<v8::Value>(v8::Undefined(isolate))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "drawLine", 6, argv, nullptr);
}

void SurfaceDrawCircleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[6] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 4 ? args[4] : v8::Local<v8::Value>(v8::Boolean::New(isolate, false)),
        args.Length() > 5 ? args[5] : v8::Local<v8::Value>(v8::Undefined(isolate))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "drawCircle", 6, argv, nullptr);
}

void SurfaceDrawTriangleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[9] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 4 ? args[4] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 5 ? args[5] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 6 ? args[6] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 7 ? args[7] : v8::Local<v8::Value>(v8::Boolean::New(isolate, false)),
        args.Length() > 8 ? args[8] : v8::Local<v8::Value>(v8::Undefined(isolate))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "drawTriangle", 9, argv, nullptr);
}

void SurfaceDrawPolylineCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[4] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Array::New(isolate)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(v8::Boolean::New(isolate, false)),
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Undefined(isolate))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "drawPolyline", 4, argv, nullptr);
}

void SurfaceFillPolygonCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[3] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Array::New(isolate)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(v8::Undefined(isolate))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "fillPolygon", 3, argv, nullptr);
}

void SurfaceDrawImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[3] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Null(isolate)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(v8::Number::New(isolate, 0))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "drawImage", 3, argv, nullptr);
}

void SurfaceResizeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int new_width = ReadInt(args, 0, 1);
    int new_height = ReadInt(args, 1, 1);
    if (new_width < 1) new_width = 1;
    if (new_height < 1) new_height = 1;
    if (new_width > 8192) new_width = 8192;
    if (new_height > 8192) new_height = 8192;

    const uint32_t fill = ReadU32(args, 2, 0);
    v8::Local<v8::Function> canvas_ctor;
    if (!GetSkiaConstructor(context, "Canvas", &canvas_ctor)) {
        return;
    }
    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, new_width),
        v8::Integer::New(isolate, new_height),
        v8::Number::New(isolate, static_cast<double>(fill))
    };
    v8::Local<v8::Object> new_canvas;
    if (!canvas_ctor->NewInstance(context, 3, ctor_args).ToLocal(&new_canvas)) {
        return;
    }
    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "_canvas"),
                           new_canvas).FromMaybe(false);
}

void SurfaceFlushCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void ImageSetPixelCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CanvasSetPixelCallback(args);
}

void CanvasToImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Function> image_ctor;
    if (!GetSkiaConstructor(context, "Image", &image_ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Array> copy = v8::Array::New(isolate, pixels->Length());
    CopyPixelArray(isolate, context, pixels, copy);

    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, width),
        v8::Integer::New(isolate, height),
        copy
    };

    v8::Local<v8::Object> image;
    if (!image_ctor->NewInstance(context, 3, ctor_args).ToLocal(&image)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    args.GetReturnValue().Set(image);
}

void SurfaceGetCanvasCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
    v8::Local<v8::Value> value;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(args.GetIsolate(), "_canvas")).ToLocal(&value)) {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
        return;
    }
    args.GetReturnValue().Set(value);
}

void SurfaceMakeImageSnapshotCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> canvas_value;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "_canvas")).ToLocal(&canvas_value)
        || !canvas_value->IsObject()) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Object> canvas = canvas_value.As<v8::Object>();
    v8::Local<v8::Value> to_image_value;
    if (!canvas->Get(context, v8::String::NewFromUtf8Literal(isolate, "toImage")).ToLocal(&to_image_value)
        || !to_image_value->IsFunction()) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Function> fn = to_image_value.As<v8::Function>();
    v8::Local<v8::Value> image;
    if (!fn->Call(context, canvas, 0, nullptr).ToLocal(&image)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    args.GetReturnValue().Set(image);
}

void SurfaceWidthCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> canvas_value;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "_canvas")).ToLocal(&canvas_value)
        || !canvas_value->IsObject()) {
        args.GetReturnValue().Set(v8::Integer::New(isolate, 0));
        return;
    }
    int width = 0;
    if (ReadIntProperty(context, canvas_value.As<v8::Object>(), "width", &width)) {
        args.GetReturnValue().Set(v8::Integer::New(isolate, width));
        return;
    }
    args.GetReturnValue().Set(v8::Integer::New(isolate, 0));
}

void SurfaceHeightCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> canvas_value;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "_canvas")).ToLocal(&canvas_value)
        || !canvas_value->IsObject()) {
        args.GetReturnValue().Set(v8::Integer::New(isolate, 0));
        return;
    }
    int height = 0;
    if (ReadIntProperty(context, canvas_value.As<v8::Object>(), "height", &height)) {
        args.GetReturnValue().Set(v8::Integer::New(isolate, height));
        return;
    }
    args.GetReturnValue().Set(v8::Integer::New(isolate, 0));
}

void ImageGetPixelCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CanvasGetPixelCallback(args);
}

void ImageCloneCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Function> image_ctor;
    if (!GetSkiaConstructor(context, "Image", &image_ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Array> copy = v8::Array::New(isolate, pixels->Length());
    CopyPixelArray(isolate, context, pixels, copy);
    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, width),
        v8::Integer::New(isolate, height),
        copy
    };
    v8::Local<v8::Object> image;
    if (!image_ctor->NewInstance(context, 3, ctor_args).ToLocal(&image)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    args.GetReturnValue().Set(image);
}

void ImageToCanvasCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Function> canvas_ctor;
    if (!GetSkiaConstructor(context, "Canvas", &canvas_ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, width),
        v8::Integer::New(isolate, height),
        v8::Number::New(isolate, 0)
    };
    v8::Local<v8::Object> canvas;
    if (!canvas_ctor->NewInstance(context, 3, ctor_args).ToLocal(&canvas)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Array> dst_pixels;
    if (!ReadArrayProperty(context, canvas, "_pixels", &dst_pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    CopyPixelArray(isolate, context, pixels, dst_pixels);
    args.GetReturnValue().Set(canvas);
}

void ImagePixelsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Array> pixels;
    if (!ReadArrayProperty(context, args.This(), "_pixels", &pixels)) {
        args.GetReturnValue().Set(v8::Array::New(isolate));
        return;
    }
    v8::Local<v8::Array> copy = v8::Array::New(isolate, pixels->Length());
    CopyPixelArray(isolate, context, pixels, copy);
    args.GetReturnValue().Set(copy);
}

void ImageWidthCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
    int width = 0;
    if (ReadIntProperty(context, args.This(), "width", &width)) {
        args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), width));
        return;
    }
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
}

void ImageHeightCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
    int height = 0;
    if (ReadIntProperty(context, args.This(), "height", &height)) {
        args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), height));
        return;
    }
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
}

void ImageToSurfaceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Function> surface_ctor;
    if (!GetSkiaConstructor(context, "Surface", &surface_ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, width),
        v8::Integer::New(isolate, height),
        v8::Number::New(isolate, 0)
    };
    v8::Local<v8::Object> surface;
    if (!surface_ctor->NewInstance(context, 3, ctor_args).ToLocal(&surface)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Value> canvas_value;
    if (!surface->Get(context, v8::String::NewFromUtf8Literal(isolate, "_canvas")).ToLocal(&canvas_value)
        || !canvas_value->IsObject()) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    v8::Local<v8::Array> dst_pixels;
    if (!ReadArrayProperty(context, canvas_value.As<v8::Object>(), "_pixels", &dst_pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    CopyPixelArray(isolate, context, pixels, dst_pixels);
    args.GetReturnValue().Set(surface);
}

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), engine::bridge::skia::Version().c_str()).ToLocalChecked());
}

void FunctionsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const auto names = engine::bridge::skia::ExportedFunctionNames();
    v8::Local<v8::Array> out = v8::Array::New(isolate, static_cast<int>(names.size()));
    for (size_t i = 0; i < names.size(); ++i) {
        (void)out->Set(context,
                       static_cast<uint32_t>(i),
                       v8::String::NewFromUtf8(isolate, names[i].c_str()).ToLocalChecked())
            .FromMaybe(false);
    }
    args.GetReturnValue().Set(out);
}

void BlendModesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const auto names = engine::bridge::skia::BlendModeNames();
    v8::Local<v8::Array> out = v8::Array::New(isolate, static_cast<int>(names.size()));
    for (size_t i = 0; i < names.size(); ++i) {
        (void)out->Set(context,
                       static_cast<uint32_t>(i),
                       v8::String::NewFromUtf8(isolate, names[i].c_str()).ToLocalChecked())
            .FromMaybe(false);
    }
    args.GetReturnValue().Set(out);
}

void HasFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string name = ReadString(isolate, args, 0);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, engine::bridge::skia::HasFunction(name)));
}

void DegToRadCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), engine::bridge::skia::DegToRad(ReadNumber(args, 0))));
}

void RadToDegCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), engine::bridge::skia::RadToDeg(ReadNumber(args, 0))));
}

void Clamp01Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), engine::bridge::skia::Clamp01(ReadNumber(args, 0))));
}

void LerpCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Number::New(
        args.GetIsolate(),
        engine::bridge::skia::Lerp(ReadNumber(args, 0), ReadNumber(args, 1), ReadNumber(args, 2))));
}

void MapRangeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Number::New(
        args.GetIsolate(),
        engine::bridge::skia::MapRange(ReadNumber(args, 0),
                                       ReadNumber(args, 1),
                                       ReadNumber(args, 2),
                                       ReadNumber(args, 3),
                                       ReadNumber(args, 4))));
}

void DistanceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Number::New(
        args.GetIsolate(),
        engine::bridge::skia::Distance(ReadNumber(args, 0),
                                       ReadNumber(args, 1),
                                       ReadNumber(args, 2),
                                       ReadNumber(args, 3))));
}

void PointToStringCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string value = engine::bridge::skia::PointToString(ReadNumber(args, 0), ReadNumber(args, 1));
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, value.c_str()).ToLocalChecked());
}

void MakeColorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const uint32_t color = engine::bridge::skia::MakeColorRGBA(
        ReadInt(args, 0),
        ReadInt(args, 1),
        ReadInt(args, 2),
        ReadInt(args, 3, 255));
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), static_cast<double>(color)));
}

void ColorToHexCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const uint32_t color = ReadU32(args, 0);
    const bool include_alpha = (args.Length() > 1 && args[1]->IsBoolean())
        ? args[1].As<v8::Boolean>()->Value()
        : true;
    const std::string out = engine::bridge::skia::ColorToHex(color, include_alpha);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, out.c_str()).ToLocalChecked());
}

void ParseColorHexCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string input = ReadString(isolate, args, 0);
    uint32_t color = 0;
    if (!engine::bridge::skia::ParseColorHex(input, &color)) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "parseColorHex expects #RRGGBB or #RRGGBBAA")));
        return;
    }
    args.GetReturnValue().Set(v8::Number::New(isolate, static_cast<double>(color)));
}

void PremultiplyAlphaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const uint32_t out = engine::bridge::skia::PremultiplyAlpha(ReadU32(args, 0));
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), static_cast<double>(out)));
}

void UnpremultiplyAlphaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const uint32_t out = engine::bridge::skia::UnpremultiplyAlpha(ReadU32(args, 0));
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), static_cast<double>(out)));
}

void NormalizeBlendModeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string mode = engine::bridge::skia::NormalizeBlendModeName(ReadString(isolate, args, 0));
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, mode.c_str()).ToLocalChecked());
}

void BlendSrcOverCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const uint32_t out = engine::bridge::skia::BlendSrcOver(ReadU32(args, 0), ReadU32(args, 1));
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), static_cast<double>(out)));
}

void BlendCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string mode = ReadString(isolate, args, 0, "srcOver");
    const uint32_t out = engine::bridge::skia::BlendModeApply(mode, ReadU32(args, 1), ReadU32(args, 2));
    args.GetReturnValue().Set(v8::Number::New(isolate, static_cast<double>(out)));
}

void MatrixIdentityCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    args.GetReturnValue().Set(MatrixToArray(isolate, context, engine::bridge::skia::MatrixIdentity()));
}

void MatrixTranslateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    args.GetReturnValue().Set(MatrixToArray(isolate,
                                            context,
                                            engine::bridge::skia::MatrixTranslate(ReadNumber(args, 0), ReadNumber(args, 1))));
}

void MatrixScaleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    args.GetReturnValue().Set(MatrixToArray(isolate,
                                            context,
                                            engine::bridge::skia::MatrixScale(ReadNumber(args, 0, 1.0), ReadNumber(args, 1, 1.0))));
}

void MatrixRotateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    args.GetReturnValue().Set(MatrixToArray(isolate, context, engine::bridge::skia::MatrixRotateDegrees(ReadNumber(args, 0))));
}

void MatrixMultiplyCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    engine::bridge::skia::Matrix3x3 a{};
    engine::bridge::skia::Matrix3x3 b{};
    v8::Local<v8::Value> arg0 = args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Undefined(isolate));
    v8::Local<v8::Value> arg1 = args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Undefined(isolate));
    if (!MatrixFromValue(isolate, context, arg0, &a)
        || !MatrixFromValue(isolate, context, arg1, &b)) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "matrixMultiply expects two number[9] arrays")));
        return;
    }

    args.GetReturnValue().Set(MatrixToArray(isolate, context, engine::bridge::skia::MatrixMultiply(a, b)));
}

void MatrixInvertCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    engine::bridge::skia::Matrix3x3 in{};
    v8::Local<v8::Value> arg0 = args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Undefined(isolate));
    if (!MatrixFromValue(isolate, context, arg0, &in)) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "matrixInvert expects number[9] array")));
        return;
    }

    engine::bridge::skia::Matrix3x3 out{};
    if (!engine::bridge::skia::MatrixInvert(in, &out)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }

    args.GetReturnValue().Set(MatrixToArray(isolate, context, out));
}

// ─── Helpers for encode/decode ───────────────────────────────────────────────

static v8::Local<v8::Value> BytesToUint8Array(v8::Isolate* isolate,
                                              const std::vector<uint8_t>& bytes) {
    const size_t len = bytes.size();
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, len);
    if (len > 0) {
        std::memcpy(ab->GetBackingStore()->Data(), bytes.data(), len);
    }
    return v8::Uint8Array::New(ab, 0, len);
}

static bool Uint8ArrayFromValue(v8::Local<v8::Context> context,
                                v8::Local<v8::Value> val,
                                std::vector<uint8_t>* out) {
    if (out == nullptr) {
        return false;
    }
    if (val->IsUint8Array() || val->IsInt8Array() || val->IsUint8ClampedArray()) {
        v8::Local<v8::TypedArray> ta = val.As<v8::TypedArray>();
        const uint8_t* ptr = static_cast<const uint8_t*>(ta->Buffer()->GetBackingStore()->Data()) + ta->ByteOffset();
        out->assign(ptr, ptr + ta->ByteLength());
        return true;
    }
    if (val->IsArray()) {
        v8::Local<v8::Array> arr = val.As<v8::Array>();
        const uint32_t len = arr->Length();
        out->resize(len);
        for (uint32_t i = 0; i < len; ++i) {
            v8::Local<v8::Value> elem;
            if (!arr->Get(context, i).ToLocal(&elem) || !elem->IsNumber()) {
                return false;
            }
            (*out)[i] = static_cast<uint8_t>(
                static_cast<int32_t>(elem->NumberValue(context).FromMaybe(0.0)) & 0xFF);
        }
        return true;
    }
    return false;
}

// ─── Path class ──────────────────────────────────────────────────────────────

void PathConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "_cmds"),
                           v8::Array::New(isolate, 0)).FromMaybe(false);
}

static void PathPushCmd(v8::Isolate* isolate,
                        v8::Local<v8::Context> context,
                        v8::Local<v8::Object> path,
                        int verb,
                        std::initializer_list<double> pts) {
    v8::Local<v8::Array> cmds;
    v8::Local<v8::Value> cmds_val;
    if (!path->Get(context, v8::String::NewFromUtf8Literal(isolate, "_cmds")).ToLocal(&cmds_val)
        || !cmds_val->IsArray()) {
        cmds = v8::Array::New(isolate, 0);
        (void)path->Set(context, v8::String::NewFromUtf8Literal(isolate, "_cmds"), cmds).FromMaybe(false);
    } else {
        cmds = cmds_val.As<v8::Array>();
    }
    v8::Local<v8::Array> pts_arr = v8::Array::New(isolate, static_cast<int>(pts.size()));
    uint32_t idx = 0;
    for (double v : pts) {
        (void)pts_arr->Set(context, idx++, v8::Number::New(isolate, v)).FromMaybe(false);
    }
    v8::Local<v8::Object> cmd_obj = v8::Object::New(isolate);
    (void)cmd_obj->Set(context, v8::String::NewFromUtf8Literal(isolate, "verb"),
                       v8::Integer::New(isolate, verb)).FromMaybe(false);
    (void)cmd_obj->Set(context, v8::String::NewFromUtf8Literal(isolate, "pts"), pts_arr).FromMaybe(false);
    (void)cmds->Set(context, cmds->Length(), cmd_obj).FromMaybe(false);
}

void PathMoveToCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    PathPushCmd(isolate, context, args.This(), 0,
                {ReadDbl(args, 0, 0.0), ReadDbl(args, 1, 0.0)});
    args.GetReturnValue().Set(args.This());
}

void PathLineToCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    PathPushCmd(isolate, context, args.This(), 1,
                {ReadDbl(args, 0, 0.0), ReadDbl(args, 1, 0.0)});
    args.GetReturnValue().Set(args.This());
}

void PathQuadToCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    PathPushCmd(isolate, context, args.This(), 2,
                {ReadDbl(args, 0, 0.0), ReadDbl(args, 1, 0.0),
                 ReadDbl(args, 2, 0.0), ReadDbl(args, 3, 0.0)});
    args.GetReturnValue().Set(args.This());
}

void PathConicToCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    PathPushCmd(isolate, context, args.This(), 3,
                {ReadDbl(args, 0, 0.0), ReadDbl(args, 1, 0.0),
                 ReadDbl(args, 2, 0.0), ReadDbl(args, 3, 0.0),
                 ReadDbl(args, 4, 1.0)});
    args.GetReturnValue().Set(args.This());
}

void PathCubicToCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    PathPushCmd(isolate, context, args.This(), 4,
                {ReadDbl(args, 0, 0.0), ReadDbl(args, 1, 0.0),
                 ReadDbl(args, 2, 0.0), ReadDbl(args, 3, 0.0),
                 ReadDbl(args, 4, 0.0), ReadDbl(args, 5, 0.0)});
    args.GetReturnValue().Set(args.This());
}

void PathArcToCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    PathPushCmd(isolate, context, args.This(), 5,
                {ReadDbl(args, 0, 0.0), ReadDbl(args, 1, 0.0),
                 ReadDbl(args, 2, 0.0), ReadDbl(args, 3, 0.0),
                 ReadDbl(args, 4, 0.0), ReadDbl(args, 5, 0.0),
                 ReadDbl(args, 6, 0.0)});
    args.GetReturnValue().Set(args.This());
}

void PathCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    PathPushCmd(isolate, context, args.This(), 6, {});
    args.GetReturnValue().Set(args.This());
}

void PathResetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    (void)args.This()->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "_cmds"),
                           v8::Array::New(isolate, 0)).FromMaybe(false);
    args.GetReturnValue().Set(args.This());
}

void PathIsEmptyCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> cmds_val;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "_cmds")).ToLocal(&cmds_val)
        || !cmds_val->IsArray()) {
        args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
        return;
    }
    args.GetReturnValue().Set(v8::Boolean::New(isolate, cmds_val.As<v8::Array>()->Length() == 0));
}

void PathCmdsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> cmds_val;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "_cmds")).ToLocal(&cmds_val)) {
        args.GetReturnValue().Set(v8::Array::New(isolate, 0));
        return;
    }
    args.GetReturnValue().Set(cmds_val);
}

void PathCloneCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Function> path_ctor;
    if (!GetSkiaConstructor(context, "Path", &path_ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Object> new_path;
    if (!path_ctor->NewInstance(context, 0, nullptr).ToLocal(&new_path)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Value> cmds_val;
    if (args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "_cmds")).ToLocal(&cmds_val)
        && cmds_val->IsArray()) {
        v8::Local<v8::Array> src = cmds_val.As<v8::Array>();
        v8::Local<v8::Array> dst = v8::Array::New(isolate, src->Length());
        for (uint32_t i = 0; i < src->Length(); ++i) {
            v8::Local<v8::Value> item;
            if (src->Get(context, i).ToLocal(&item)) {
                (void)dst->Set(context, i, item).FromMaybe(false);
            }
        }
        (void)new_path->Set(context,
                            v8::String::NewFromUtf8Literal(isolate, "_cmds"),
                            dst).FromMaybe(false);
    }
    args.GetReturnValue().Set(new_path);
}

static bool PathDataFromJSPath(v8::Local<v8::Context> context,
                                v8::Local<v8::Object> path_obj,
                                engine::bridge::skia::PathData* out) {
    v8::Isolate* isolate = context->GetIsolate();
    v8::Local<v8::Value> cmds_val;
    if (!path_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "_cmds")).ToLocal(&cmds_val)
        || !cmds_val->IsArray()) {
        return false;
    }
    v8::Local<v8::Array> cmds = cmds_val.As<v8::Array>();
    out->clear();
    out->reserve(cmds->Length());
    for (uint32_t i = 0; i < cmds->Length(); ++i) {
        v8::Local<v8::Value> cmd_val;
        if (!cmds->Get(context, i).ToLocal(&cmd_val) || !cmd_val->IsObject()) {
            continue;
        }
        v8::Local<v8::Object> cmd_obj = cmd_val.As<v8::Object>();
        v8::Local<v8::Value> verb_val;
        if (!cmd_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "verb")).ToLocal(&verb_val)
            || !verb_val->IsNumber()) {
            continue;
        }
        const int verb_int = static_cast<int>(verb_val->NumberValue(context).FromMaybe(0.0));
        std::vector<float> pts;
        v8::Local<v8::Value> pv;
        if (cmd_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "pts")).ToLocal(&pv)
            && pv->IsArray()) {
            v8::Local<v8::Array> pts_arr = pv.As<v8::Array>();
            pts.reserve(pts_arr->Length());
            for (uint32_t j = 0; j < pts_arr->Length(); ++j) {
                v8::Local<v8::Value> pval;
                pts.push_back(pts_arr->Get(context, j).ToLocal(&pval) && pval->IsNumber()
                              ? static_cast<float>(pval->NumberValue(context).FromMaybe(0.0))
                              : 0.f);
            }
        }
        engine::bridge::skia::PathVerb verb{};
        switch (verb_int) {
            case 0: verb = engine::bridge::skia::PathVerb::kMove;  break;
            case 1: verb = engine::bridge::skia::PathVerb::kLine;  break;
            case 2: verb = engine::bridge::skia::PathVerb::kQuad;  break;
            case 3: verb = engine::bridge::skia::PathVerb::kConic; break;
            case 4: verb = engine::bridge::skia::PathVerb::kCubic; break;
            case 5: verb = engine::bridge::skia::PathVerb::kArcTo; break;
            case 6: verb = engine::bridge::skia::PathVerb::kClose; break;
            default: continue;
        }
        out->push_back({verb, std::move(pts)});
    }
    return !out->empty();
}

// ─── Canvas.drawPath / clipRect / save / restore / encode ────────────────────

void CanvasDrawPathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    int width = 0;
    int height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }
    if (args.Length() < 1 || !args[0]->IsObject()) {
        return;
    }
    v8::Local<v8::Object> path_obj = args[0].As<v8::Object>();

    PaintState paint;
    if (args.Length() >= 2 && args[1]->IsObject()) {
        paint = ReadPaintState(isolate, context, args[1], 0xFFFFFFFF, true);
    } else {
        paint.color = ReadU32(args, 1, 0xFFFFFFFF);
        paint.fill  = ReadBool(args, 2, true);
        paint.blend_mode = "srcOver";
    }

    if (engine::bridge::skia::HasNativeRaster()) {
        engine::bridge::skia::PathData path_data;
        std::vector<uint32_t> native;
        if (PathDataFromJSPath(context, path_obj, &path_data)
            && PixelArrayToVector(context, pixels, width, height, &native)
            && engine::bridge::skia::RasterDrawPath(&native, width, height,
                                                    path_data, paint.color,
                                                    paint.blend_mode, paint.fill)) {
            PixelVectorToArray(isolate, context, native, pixels);
            return;
        }
    }

    // CPU fallback: moveTo/lineTo/close via Bresenham
    v8::Local<v8::Value> cmds_val;
    if (!path_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "_cmds")).ToLocal(&cmds_val)
        || !cmds_val->IsArray()) {
        return;
    }
    v8::Local<v8::Array> cmds = cmds_val.As<v8::Array>();
    int cx = 0, cy = 0, mx = 0, my = 0;
    for (uint32_t i = 0; i < cmds->Length(); ++i) {
        v8::Local<v8::Value> cv;
        if (!cmds->Get(context, i).ToLocal(&cv) || !cv->IsObject()) {
            continue;
        }
        v8::Local<v8::Object> cmd = cv.As<v8::Object>();
        v8::Local<v8::Value> vv;
        if (!cmd->Get(context, v8::String::NewFromUtf8Literal(isolate, "verb")).ToLocal(&vv) || !vv->IsNumber()) {
            continue;
        }
        const int verb = static_cast<int>(vv->NumberValue(context).FromMaybe(0.0));
        v8::Local<v8::Value> pv;
        bool has_pts = cmd->Get(context, v8::String::NewFromUtf8Literal(isolate, "pts")).ToLocal(&pv) && pv->IsArray();
        v8::Local<v8::Array> pts_arr = has_pts ? pv.As<v8::Array>() : v8::Array::New(isolate, 0);
        auto get_pt = [&](uint32_t j) -> int {
            if (!has_pts || j >= pts_arr->Length()) { return 0; }
            v8::Local<v8::Value> pval;
            if (!pts_arr->Get(context, j).ToLocal(&pval)) { return 0; }
            return static_cast<int>(pval->NumberValue(context).FromMaybe(0.0));
        };
        if (verb == 0) {
            cx = get_pt(0); cy = get_pt(1);
            mx = cx; my = cy;
        } else if (verb == 1) {
            const int nx = get_pt(0), ny = get_pt(1);
            DrawLineWithPaint(isolate, context, pixels, width, height, cx, cy, nx, ny, paint);
            cx = nx; cy = ny;
        } else if (verb == 6) {
            DrawLineWithPaint(isolate, context, pixels, width, height, cx, cy, mx, my, paint);
            cx = mx; cy = my;
        }
    }
}

void CanvasClipRectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0, height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }
    const int cx = ReadInt(args, 0, 0);
    const int cy = ReadInt(args, 1, 0);
    const int cw = ReadInt(args, 2, width);
    const int ch = ReadInt(args, 3, height);
    std::vector<uint32_t> native;
    if (!PixelArrayToVector(context, pixels, width, height, &native)) {
        return;
    }
    std::vector<uint32_t> saved;
    if (!engine::bridge::skia::RasterClipRect(&native, width, height, cx, cy, cw, ch, &saved)) {
        return;
    }
    PixelVectorToArray(isolate, context, native, pixels);
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, saved.size() * 4);
    std::memcpy(ab->GetBackingStore()->Data(), saved.data(), saved.size() * 4);
    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "_clip_save"), ab).FromMaybe(false);
}

void CanvasClipRestoreCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0, height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }
    v8::Local<v8::Value> save_val;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "_clip_save")).ToLocal(&save_val)
        || !save_val->IsArrayBuffer()) {
        return;
    }
    v8::Local<v8::ArrayBuffer> ab = save_val.As<v8::ArrayBuffer>();
    const size_t count = ab->ByteLength() / 4;
    std::vector<uint32_t> saved(count);
    std::memcpy(saved.data(), ab->GetBackingStore()->Data(), count * 4);
    std::vector<uint32_t> native;
    if (!PixelArrayToVector(context, pixels, width, height, &native)) {
        return;
    }
    if (engine::bridge::skia::RasterClipRestore(&native, width, height, saved)) {
        PixelVectorToArray(isolate, context, native, pixels);
    }
}

void CanvasSaveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0, height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }
    std::vector<uint32_t> native;
    if (!PixelArrayToVector(context, pixels, width, height, &native)) {
        return;
    }
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, native.size() * 4);
    std::memcpy(ab->GetBackingStore()->Data(), native.data(), native.size() * 4);
    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "_save"), ab).FromMaybe(false);
}

void CanvasRestoreCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0, height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        return;
    }
    v8::Local<v8::Value> save_val;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "_save")).ToLocal(&save_val)
        || !save_val->IsArrayBuffer()) {
        return;
    }
    v8::Local<v8::ArrayBuffer> ab = save_val.As<v8::ArrayBuffer>();
    const size_t count = ab->ByteLength() / 4;
    if (count != static_cast<size_t>(width * height)) {
        return;
    }
    std::vector<uint32_t> saved(count);
    std::memcpy(saved.data(), ab->GetBackingStore()->Data(), count * 4);
    PixelVectorToArray(isolate, context, saved, pixels);
}

void CanvasEncodePNGCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0, height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    std::vector<uint32_t> native;
    if (!PixelArrayToVector(context, pixels, width, height, &native)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    std::vector<uint8_t> out_bytes;
    if (!engine::bridge::skia::EncodeImagePNG(native, width, height, &out_bytes)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    args.GetReturnValue().Set(BytesToUint8Array(isolate, out_bytes));
}

void CanvasEncodeJPEGCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0, height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    const int quality = ReadInt(args, 0, 90);
    std::vector<uint32_t> native;
    if (!PixelArrayToVector(context, pixels, width, height, &native)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    std::vector<uint8_t> out_bytes;
    if (!engine::bridge::skia::EncodeImageJPEG(native, width, height, quality, &out_bytes)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    args.GetReturnValue().Set(BytesToUint8Array(isolate, out_bytes));
}

// ─── Image.encodePNG / encodeJPEG ────────────────────────────────────────────

void ImageEncodePNGCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CanvasEncodePNGCallback(args);
}

void ImageEncodeJPEGCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CanvasEncodeJPEGCallback(args);
}

// ─── Skia.decodeImage ────────────────────────────────────────────────────────

void DecodeImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    std::vector<uint8_t> bytes;
    if (!Uint8ArrayFromValue(context, args[0], &bytes) || bytes.empty()) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    int out_w = 0;
    int out_h = 0;
    std::vector<uint32_t> out_pixels;
    if (!engine::bridge::skia::DecodeImage(bytes, &out_w, &out_h, &out_pixels)
        || out_w <= 0 || out_h <= 0) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Function> image_ctor;
    if (!GetSkiaConstructor(context, "Image", &image_ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Value> ctor_args[3] = {
        v8::Integer::New(isolate, out_w),
        v8::Integer::New(isolate, out_h),
        v8::Number::New(isolate, 0.0)
    };
    v8::Local<v8::Object> img;
    if (!image_ctor->NewInstance(context, 3, ctor_args).ToLocal(&img)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Array> dst_pixels;
    if (!ReadArrayProperty(context, img, "_pixels", &dst_pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    PixelVectorToArray(isolate, context, out_pixels, dst_pixels);
    args.GetReturnValue().Set(img);
}

// ─── Surface proxies for new Canvas methods ──────────────────────────────────

void SurfaceDrawPathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[2] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Null(isolate)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Undefined(isolate))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "drawPath", 2, argv, nullptr);
}

void SurfaceClipRectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[4] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 2 ? args[2] : v8::Local<v8::Value>(v8::Number::New(isolate, 0)),
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Number::New(isolate, 0))
    };
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "clipRect", 4, argv, nullptr);
}

void SurfaceClipRestoreCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "clipRestore", 0, nullptr, nullptr);
}

void SurfaceSaveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "save", 0, nullptr, nullptr);
}

void SurfaceRestoreCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    (void)CallSurfaceCanvasMethod(isolate, context, args.This(), "restore", 0, nullptr, nullptr);
}

void SurfaceEncodePNGCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> out;
    if (!CallSurfaceCanvasMethod(isolate, context, args.This(), "encodePNG", 0, nullptr, &out)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    args.GetReturnValue().Set(out);
}

void SurfaceEncodeJPEGCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> argv[1] = {
        args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Number::New(isolate, 90))
    };
    v8::Local<v8::Value> out;
    if (!CallSurfaceCanvasMethod(isolate, context, args.This(), "encodeJPEG", 1, argv, &out)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    args.GetReturnValue().Set(out);
}


// ════════════════════════════════════════════════════════════════════════════
// Shared helper: read pixels into vector, call bridge fn, write back
// ════════════════════════════════════════════════════════════════════════════

// Convenience macro-like helper: read → call → writeback
#define PS_CANVAS_OP(obj, context, w, h, pixels, native, expr) \
    do { \
        int w = 0, h = 0; \
        v8::Local<v8::Array> pixels; \
        if (!ReadCanvasData((context), (obj), &(w), &(h), &(pixels))) return; \
        std::vector<uint32_t> native; \
        if (!PixelArrayToVector((context), (pixels), (w), (h), &(native))) return; \
        (expr); \
        PixelVectorToArray((context)->GetIsolate(), (context), (native), (pixels)); \
    } while(0)

// ─── LUT helper: read uint8 array from JS value ──────────────────────────────
static bool ReadLUT256(v8::Local<v8::Context> ctx, v8::Local<v8::Value> val,
                       std::array<uint8_t, 256>* lut) {
    if (val->IsNull() || val->IsUndefined()) return false;
    if (val->IsArray()) {
        v8::Local<v8::Array> arr = val.As<v8::Array>();
        if (arr->Length() < 256) return false;
        for (int i = 0; i < 256; ++i) {
            v8::Local<v8::Value> ev;
            if (!arr->Get(ctx, static_cast<uint32_t>(i)).ToLocal(&ev)) return false;
            (*lut)[static_cast<size_t>(i)] = static_cast<uint8_t>(
                static_cast<int>(ev->NumberValue(ctx).FromMaybe(0.0)) & 0xFF);
        }
        return true;
    }
    if (val->IsUint8Array()) {
        v8::Local<v8::Uint8Array> ta = val.As<v8::Uint8Array>();
        if (ta->Length() < 256) return false;
        const uint8_t* ptr = static_cast<const uint8_t*>(
            ta->Buffer()->GetBackingStore()->Data()) + ta->ByteOffset();
        std::memcpy(lut->data(), ptr, 256);
        return true;
    }
    return false;
}

// ─── GradientStops from JS array ─────────────────────────────────────────────
static engine::bridge::skia::GradientStops ReadGradientStops(
    v8::Isolate* isolate, v8::Local<v8::Context> ctx, v8::Local<v8::Value> val) {
    engine::bridge::skia::GradientStops stops;
    if (!val->IsArray()) return stops;
    v8::Local<v8::Array> arr = val.As<v8::Array>();
    for (uint32_t i = 0; i < arr->Length(); ++i) {
        v8::Local<v8::Value> item;
        if (!arr->Get(ctx, i).ToLocal(&item) || !item->IsObject()) continue;
        v8::Local<v8::Object> obj = item.As<v8::Object>();
        v8::Local<v8::Value> pos_v, col_v;
        float pos = 0.f;
        uint32_t col = 0xFF000000u;
        if (obj->Get(ctx, v8::String::NewFromUtf8Literal(isolate, "pos")).ToLocal(&pos_v) && pos_v->IsNumber())
            pos = static_cast<float>(pos_v->NumberValue(ctx).FromMaybe(0.0));
        if (obj->Get(ctx, v8::String::NewFromUtf8Literal(isolate, "color")).ToLocal(&col_v) && col_v->IsNumber())
            col = static_cast<uint32_t>(col_v->NumberValue(ctx).FromMaybe(0.0));
        stops.push_back({pos, col});
    }
    return stops;
}

// ─── FontOptions from JS object ──────────────────────────────────────────────
static engine::bridge::skia::FontOptions ReadFontOptions(
    v8::Isolate* isolate, v8::Local<v8::Context> ctx, v8::Local<v8::Value> val) {
    engine::bridge::skia::FontOptions f;
    f.size = 16.0;
    f.weight = 400;
    f.italic = false;
    f.underline = false;
    f.strikethrough = false;
    if (!val->IsObject()) return f;
    v8::Local<v8::Object> obj = val.As<v8::Object>();
    auto get_str = [&](const char* key) -> std::string {
        v8::Local<v8::Value> v;
        if (!obj->Get(ctx, v8::String::NewFromUtf8(isolate, key).ToLocalChecked()).ToLocal(&v)) return "";
        if (!v->IsString()) return "";
        v8::String::Utf8Value u(isolate, v);
        return std::string(*u, u.length());
    };
    auto get_dbl = [&](const char* key, double def) -> double {
        v8::Local<v8::Value> v;
        if (!obj->Get(ctx, v8::String::NewFromUtf8(isolate, key).ToLocalChecked()).ToLocal(&v)) return def;
        if (!v->IsNumber()) return def;
        return v->NumberValue(ctx).FromMaybe(def);
    };
    auto get_bool = [&](const char* key, bool def) -> bool {
        v8::Local<v8::Value> v;
        if (!obj->Get(ctx, v8::String::NewFromUtf8(isolate, key).ToLocalChecked()).ToLocal(&v)) return def;
        return v->BooleanValue(isolate);
    };
    f.family = get_str("family");
    f.size   = get_dbl("size", 16.0);
    f.weight = static_cast<int>(get_dbl("weight", 400.0));
    f.italic = get_bool("italic", false);
    f.underline = get_bool("underline", false);
    f.strikethrough = get_bool("strikethrough", false);
    return f;
}

// ════════════════════════════════════════════════════════════════════════════
// Transform callbacks (Canvas / Bitmap)
// ════════════════════════════════════════════════════════════════════════════

// canvas.scale(dstW, dstH, bilinear=true) → modifies in-place (resizes)
void CanvasScaleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0, height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) return;
    const int dw = ReadInt(args, 0, width);
    const int dh = ReadInt(args, 1, height);
    const bool bilinear = args.Length() < 3 || args[2]->BooleanValue(isolate);
    std::vector<uint32_t> src;
    if (!PixelArrayToVector(context, pixels, width, height, &src)) return;
    std::vector<uint32_t> dst;
    if (!engine::bridge::skia::ImageScale(src, width, height, dw, dh, &dst, bilinear)) return;
    // Resize JS array
    while (static_cast<int>(pixels->Length()) > dw * dh) {
        (void)pixels->Delete(context, pixels->Length() - 1).FromMaybe(false);
    }
    PixelVectorToArray(isolate, context, dst, pixels);
    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "_width"),
                           v8::Integer::New(isolate, dw)).FromMaybe(false);
    (void)args.This()->Set(context, v8::String::NewFromUtf8Literal(isolate, "_height"),
                           v8::Integer::New(isolate, dh)).FromMaybe(false);
}

// canvas.rotate(degrees, bgColor=0) → returns new Canvas with rotated content
void CanvasRotateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int width = 0, height = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &width, &height, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    const double deg = ReadDbl(args, 0, 0.0);
    const uint32_t bg = ReadU32(args, 1, 0u);
    std::vector<uint32_t> src;
    if (!PixelArrayToVector(context, pixels, width, height, &src)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    std::vector<uint32_t> out;
    int ow = 0, oh = 0;
    if (!engine::bridge::skia::ImageRotate(src, width, height, deg, bg, &out, &ow, &oh)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Function> ctor;
    if (!GetSkiaConstructor(context, "Canvas", &ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Value> cargs[2] = {v8::Integer::New(isolate, ow), v8::Integer::New(isolate, oh)};
    v8::Local<v8::Object> new_canvas;
    if (!ctor->NewInstance(context, 2, cargs).ToLocal(&new_canvas)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Array> dst_pixels;
    if (!ReadArrayProperty(context, new_canvas, "_pixels", &dst_pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    PixelVectorToArray(isolate, context, out, dst_pixels);
    args.GetReturnValue().Set(new_canvas);
}

void CanvasFlipHCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int w = 0, h = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> src, dst;
    if (!PixelArrayToVector(context, pixels, w, h, &src)) return;
    if (!engine::bridge::skia::ImageFlipH(src, w, h, &dst)) return;
    PixelVectorToArray(isolate, context, dst, pixels);
}

void CanvasFlipVCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int w = 0, h = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> src, dst;
    if (!PixelArrayToVector(context, pixels, w, h, &src)) return;
    if (!engine::bridge::skia::ImageFlipV(src, w, h, &dst)) return;
    PixelVectorToArray(isolate, context, dst, pixels);
}

void CanvasCropCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int w = 0, h = 0;
    v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(context, args.This(), &w, &h, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    const int x = ReadInt(args, 0, 0), y = ReadInt(args, 1, 0);
    const int cw = ReadInt(args, 2, w),  ch = ReadInt(args, 3, h);
    std::vector<uint32_t> src, out;
    if (!PixelArrayToVector(context, pixels, w, h, &src)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    if (!engine::bridge::skia::ImageCrop(src, w, h, x, y, cw, ch, &out)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    const int ow = std::min(w - std::max(0, x), cw);
    const int oh = std::min(h - std::max(0, y), ch);
    v8::Local<v8::Function> ctor;
    if (!GetSkiaConstructor(context, "Canvas", &ctor)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Value> cargs[2] = {v8::Integer::New(isolate, ow), v8::Integer::New(isolate, oh)};
    v8::Local<v8::Object> new_canvas;
    if (!ctor->NewInstance(context, 2, cargs).ToLocal(&new_canvas)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::Local<v8::Array> dst_p;
    if (!ReadArrayProperty(context, new_canvas, "_pixels", &dst_p)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    PixelVectorToArray(isolate, context, out, dst_p);
    args.GetReturnValue().Set(new_canvas);
}

void CanvasCompositeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int dw = 0, dh = 0;
    v8::Local<v8::Array> dpix;
    if (!ReadCanvasData(context, args.This(), &dw, &dh, &dpix)) return;
    if (args.Length() < 1 || !args[0]->IsObject()) return;
    v8::Local<v8::Object> src_obj = args[0].As<v8::Object>();
    int sw = 0, sh = 0;
    v8::Local<v8::Array> spix;
    if (!ReadCanvasData(context, src_obj, &sw, &sh, &spix)) return;
    const int dx = ReadInt(args, 1, 0), dy = ReadInt(args, 2, 0);
    std::string blend = "srcOver";
    if (args.Length() >= 4 && args[3]->IsString()) {
        v8::String::Utf8Value u(isolate, args[3]);
        blend = std::string(*u, u.length());
    }
    const float alpha = static_cast<float>(ReadDbl(args, 4, 1.0));
    std::vector<uint32_t> dst_v, src_v;
    if (!PixelArrayToVector(context, dpix, dw, dh, &dst_v)) return;
    if (!PixelArrayToVector(context, spix, sw, sh, &src_v)) return;
    engine::bridge::skia::ImageComposite(&dst_v, dw, dh, src_v, sw, sh, dx, dy, blend, alpha);
    PixelVectorToArray(isolate, context, dst_v, dpix);
}

// ════════════════════════════════════════════════════════════════════════════
// Colour adjustment callbacks
// ════════════════════════════════════════════════════════════════════════════

void CanvasAdjustBrightnessContrastCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustBrightnessContrast(&v, w, h,
        ReadInt(args, 0, 0), ReadInt(args, 1, 0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasAdjustHSLCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustHSL(&v, w, h,
        ReadDbl(args, 0, 0.0), ReadDbl(args, 1, 0.0), ReadDbl(args, 2, 0.0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasAdjustExposureCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustExposure(&v, w, h,
        ReadDbl(args, 0, 0.0), ReadDbl(args, 1, 1.0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasAdjustLevelsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustLevels(&v, w, h,
        ReadInt(args, 0, 0), ReadInt(args, 1, 255), ReadDbl(args, 2, 1.0),
        ReadInt(args, 3, 0), ReadInt(args, 4, 255));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasAdjustCurvesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    std::array<uint8_t, 256> lr{}, lg{}, lb{}, la{};
    const uint8_t* pr = nullptr, *pg = nullptr, *pb = nullptr, *pa = nullptr;
    if (args.Length() > 0 && ReadLUT256(ctx, args[0], &lr)) pr = lr.data();
    if (args.Length() > 1 && ReadLUT256(ctx, args[1], &lg)) pg = lg.data();
    if (args.Length() > 2 && ReadLUT256(ctx, args[2], &lb)) pb = lb.data();
    if (args.Length() > 3 && ReadLUT256(ctx, args[3], &la)) pa = la.data();
    engine::bridge::skia::AdjustCurves(&v, w, h, pr, pg, pb, pa);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasAdjustColorBalanceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustColorBalance(&v, w, h,
        ReadInt(args,0,0), ReadInt(args,1,0), ReadInt(args,2,0),
        ReadInt(args,3,0), ReadInt(args,4,0), ReadInt(args,5,0),
        ReadInt(args,6,0), ReadInt(args,7,0), ReadInt(args,8,0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasAdjustVibranceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustVibrance(&v, w, h, ReadDbl(args, 0, 0.0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasAdjustChannelMixerCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    // args: rr,rg,rb,rc, gr,gg,gb,gc, br,bg,bb,bc
    engine::bridge::skia::ChannelMixerCoeff cr{
        ReadDbl(args,0,1.0), ReadDbl(args,1,0.0), ReadDbl(args,2,0.0), ReadDbl(args,3,0.0)};
    engine::bridge::skia::ChannelMixerCoeff cg{
        ReadDbl(args,4,0.0), ReadDbl(args,5,1.0), ReadDbl(args,6,0.0), ReadDbl(args,7,0.0)};
    engine::bridge::skia::ChannelMixerCoeff cb2{
        ReadDbl(args,8,0.0), ReadDbl(args,9,0.0), ReadDbl(args,10,1.0), ReadDbl(args,11,0.0)};
    engine::bridge::skia::AdjustChannelMixer(&v, w, h, cr, cg, cb2);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasColorReplaceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::ColorReplace(&v, w, h,
        ReadU32(args, 0, 0u), ReadU32(args, 1, 0xFFFFFFFFu), ReadInt(args, 2, 10));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasInvertCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustInvert(&v, w, h);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasGrayscaleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustGrayscale(&v, w, h);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasSepiaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustSepia(&v, w, h, ReadDbl(args, 0, 1.0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasThresholdCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustThreshold(&v, w, h, ReadInt(args, 0, 128));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasPosterizeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustPosterize(&v, w, h, ReadInt(args, 0, 4));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasOpacityCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AdjustOpacity(&v, w, h, static_cast<float>(ReadDbl(args, 0, 1.0)));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasSelectiveColorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    std::string range = "all";
    if (args.Length() > 0 && args[0]->IsString()) {
        v8::String::Utf8Value u(isolate, args[0]);
        range = std::string(*u, u.length());
    }
    engine::bridge::skia::AdjustSelectiveColor(&v, w, h, range,
        ReadInt(args,1,0), ReadInt(args,2,0), ReadInt(args,3,0), ReadInt(args,4,0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

// ════════════════════════════════════════════════════════════════════════════
// Filter callbacks
// ════════════════════════════════════════════════════════════════════════════

void CanvasFilterBlurCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    const int rx = ReadInt(args, 0, 3);
    const int ry = ReadInt(args, 1, rx);
    engine::bridge::skia::FilterBlur(&v, w, h, rx, ry);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterGaussianBlurCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    const double sx = ReadDbl(args, 0, 1.0);
    const double sy = ReadDbl(args, 1, sx);
    engine::bridge::skia::FilterGaussianBlur(&v, w, h, sx, sy);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterMotionBlurCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterMotionBlur(&v, w, h,
        ReadDbl(args, 0, 0.0), ReadInt(args, 1, 10));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterRadialBlurCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterRadialBlur(&v, w, h,
        ReadDbl(args, 0, 10.0), ReadBool(args, 1, false));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterSharpenCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterSharpen(&v, w, h,
        ReadDbl(args, 0, 1.0), ReadDbl(args, 1, 1.0), ReadInt(args, 2, 0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterEmbossCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterEmboss(&v, w, h,
        ReadDbl(args, 0, 315.0), ReadDbl(args, 1, 1.0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterEdgeDetectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterEdgeDetect(&v, w, h);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterAddNoiseCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterAddNoise(&v, w, h,
        ReadInt(args, 0, 25), ReadBool(args, 1, false), ReadBool(args, 2, true));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterMedianCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterMedian(&v, w, h, ReadInt(args, 0, 2));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterPixelateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterPixelate(&v, w, h, ReadInt(args, 0, 8));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterOilPaintCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterOilPaint(&v, w, h,
        ReadInt(args, 0, 4), ReadInt(args, 1, 20));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterVignetteCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterVignette(&v, w, h,
        ReadDbl(args, 0, 0.5), ReadDbl(args, 1, 0.3));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterChromaticAberrationCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterChromaticAberration(&v, w, h,
        ReadInt(args,0, 3), ReadInt(args,1, 0),
        ReadInt(args,2,-3), ReadInt(args,3, 0));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFilterConvolveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    if (args.Length() < 1 || !args[0]->IsArray()) return;
    v8::Local<v8::Array> karr = args[0].As<v8::Array>();
    const int kw = ReadInt(args, 1, static_cast<int>(std::sqrt(karr->Length())));
    const int kh = ReadInt(args, 2, static_cast<int>(std::sqrt(karr->Length())));
    const double div = ReadDbl(args, 3, 1.0);
    const double bias = ReadDbl(args, 4, 0.0);
    std::vector<double> kernel(karr->Length());
    for (uint32_t i = 0; i < karr->Length(); ++i) {
        v8::Local<v8::Value> kv;
        kernel[i] = karr->Get(ctx, i).ToLocal(&kv) && kv->IsNumber()
            ? kv->NumberValue(ctx).FromMaybe(0.0) : 0.0;
    }
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::FilterConvolve(&v, w, h, kernel, kw, kh, div, bias);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

// ════════════════════════════════════════════════════════════════════════════
// Drawing enhancement callbacks
// ════════════════════════════════════════════════════════════════════════════

void CanvasLinearGradientCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    const auto stops = ReadGradientStops(isolate, ctx,
        args.Length() > 4 ? args[4] : v8::Local<v8::Value>(v8::Array::New(isolate, 0)));
    std::string blend = "srcOver";
    if (args.Length() > 5 && args[5]->IsString()) {
        v8::String::Utf8Value u(isolate, args[5]);
        blend = std::string(*u, u.length());
    }
    engine::bridge::skia::RasterLinearGradient(&v, w, h,
        static_cast<float>(ReadDbl(args,0,0.0)),
        static_cast<float>(ReadDbl(args,1,0.0)),
        static_cast<float>(ReadDbl(args,2,static_cast<double>(w))),
        static_cast<float>(ReadDbl(args,3,0.0)),
        stops, blend);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasRadialGradientCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    const auto stops = ReadGradientStops(isolate, ctx,
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Array::New(isolate, 0)));
    std::string blend = "srcOver";
    if (args.Length() > 4 && args[4]->IsString()) {
        v8::String::Utf8Value u(isolate, args[4]);
        blend = std::string(*u, u.length());
    }
    engine::bridge::skia::RasterRadialGradient(&v, w, h,
        static_cast<float>(ReadDbl(args,0,static_cast<double>(w)/2)),
        static_cast<float>(ReadDbl(args,1,static_cast<double>(h)/2)),
        static_cast<float>(ReadDbl(args,2,static_cast<double>(std::min(w,h))/2)),
        stops, blend);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasSweepGradientCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    const auto stops = ReadGradientStops(isolate, ctx,
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Array::New(isolate, 0)));
    std::string blend = "srcOver";
    if (args.Length() > 4 && args[4]->IsString()) {
        v8::String::Utf8Value u(isolate, args[4]);
        blend = std::string(*u, u.length());
    }
    engine::bridge::skia::RasterSweepGradient(&v, w, h,
        static_cast<float>(ReadDbl(args,0,static_cast<double>(w)/2)),
        static_cast<float>(ReadDbl(args,1,static_cast<double>(h)/2)),
        static_cast<float>(ReadDbl(args,2,0.0)),
        stops, blend);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFloodFillCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::RasterFloodFill(&v, w, h,
        ReadInt(args,0,0), ReadInt(args,1,0),
        ReadU32(args,2,0xFFFFFFFFu), ReadInt(args,3,15));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasDrawRoundRectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    std::string blend = "srcOver";
    if (args.Length() > 7 && args[7]->IsString()) {
        v8::String::Utf8Value u(isolate, args[7]);
        blend = std::string(*u, u.length());
    }
    engine::bridge::skia::RasterDrawRoundRect(&v, w, h,
        ReadInt(args,0,0), ReadInt(args,1,0),
        ReadInt(args,2,w), ReadInt(args,3,h),
        ReadInt(args,4,8), ReadInt(args,5,8),
        ReadU32(args,6,0xFFFFFFFFu), blend,
        ReadBool(args,8,true));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasDrawEllipseCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    std::string blend = "srcOver";
    if (args.Length() > 5 && args[5]->IsString()) {
        v8::String::Utf8Value u(isolate, args[5]);
        blend = std::string(*u, u.length());
    }
    engine::bridge::skia::RasterDrawEllipse(&v, w, h,
        ReadInt(args,0,w/2), ReadInt(args,1,h/2),
        ReadInt(args,2,w/4), ReadInt(args,3,h/4),
        ReadU32(args,4,0xFFFFFFFFu), blend,
        ReadBool(args,6,true));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasDrawArcCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    std::string blend = "srcOver";
    if (args.Length() > 7 && args[7]->IsString()) {
        v8::String::Utf8Value u(isolate, args[7]);
        blend = std::string(*u, u.length());
    }
    engine::bridge::skia::RasterDrawArc(&v, w, h,
        ReadInt(args,0,w/2), ReadInt(args,1,h/2),
        ReadInt(args,2,w/4), ReadInt(args,3,h/4),
        ReadDbl(args,4,0.0), ReadDbl(args,5,360.0),
        ReadU32(args,6,0xFFFFFFFFu), blend);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasDrawThickLineCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    std::string blend = "srcOver";
    if (args.Length() > 6 && args[6]->IsString()) {
        v8::String::Utf8Value u(isolate, args[6]);
        blend = std::string(*u, u.length());
    }
    engine::bridge::skia::RasterDrawThickLine(&v, w, h,
        static_cast<float>(ReadDbl(args,0,0.0)),
        static_cast<float>(ReadDbl(args,1,0.0)),
        static_cast<float>(ReadDbl(args,2,static_cast<double>(w))),
        static_cast<float>(ReadDbl(args,3,0.0)),
        static_cast<float>(ReadDbl(args,4,2.0)),
        ReadU32(args,5,0xFFFFFFFFu), blend);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasDropShadowCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> src;
    if (!PixelArrayToVector(ctx, pixels, w, h, &src)) return;
    // Create dst same size filled transparent
    std::vector<uint32_t> dst(static_cast<size_t>(w * h), 0u);
    engine::bridge::skia::RasterDropShadow(&dst, w, h, src, w, h,
        ReadInt(args,0,4), ReadInt(args,1,4),
        ReadDbl(args,2,3.0), ReadU32(args,3,0x000000FFu));
    // Composite original on top
    engine::bridge::skia::ImageComposite(&dst, w, h, src, w, h, 0, 0, "srcOver", 1.0f);
    PixelVectorToArray(isolate, ctx, dst, pixels);
}

void CanvasGlowCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::RasterGlow(&v, w, h,
        ReadDbl(args,0,5.0), ReadU32(args,1,0xFFFFFF80u),
        ReadBool(args,2,false), static_cast<float>(ReadDbl(args,3,1.0)));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

// ════════════════════════════════════════════════════════════════════════════
// Text callbacks
// ════════════════════════════════════════════════════════════════════════════

void CanvasDrawTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    if (args.Length() < 1 || !args[0]->IsString()) return;
    v8::String::Utf8Value text_u(isolate, args[0]);
    std::string text(*text_u, text_u.length());
    const float x = static_cast<float>(ReadDbl(args, 1, 0.0));
    const float y = static_cast<float>(ReadDbl(args, 2, 16.0));
    const auto font = ReadFontOptions(isolate, ctx,
        args.Length() > 3 ? args[3] : v8::Local<v8::Value>(v8::Undefined(isolate)));
    const uint32_t color = ReadU32(args, 4, 0xFFFFFFFFu);
    std::string blend = "srcOver";
    if (args.Length() > 5 && args[5]->IsString()) {
        v8::String::Utf8Value u(isolate, args[5]);
        blend = std::string(*u, u.length());
    }
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::RasterDrawText(&v, w, h, text, x, y, font, color, blend);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void SkiaMeasureTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    v8::String::Utf8Value text_u(isolate, args[0]);
    std::string text(*text_u, text_u.length());
    const auto font = ReadFontOptions(isolate, ctx,
        args.Length() > 1 ? args[1] : v8::Local<v8::Value>(v8::Undefined(isolate)));
    float tw = 0.f, th = 0.f, tb = 0.f;
    engine::bridge::skia::MeasureText(text, font, &tw, &th, &tb);
    v8::Local<v8::Object> result = v8::Object::New(isolate);
    (void)result->Set(ctx, v8::String::NewFromUtf8Literal(isolate, "width"),
                      v8::Number::New(isolate, tw)).FromMaybe(false);
    (void)result->Set(ctx, v8::String::NewFromUtf8Literal(isolate, "height"),
                      v8::Number::New(isolate, th)).FromMaybe(false);
    (void)result->Set(ctx, v8::String::NewFromUtf8Literal(isolate, "baseline"),
                      v8::Number::New(isolate, tb)).FromMaybe(false);
    args.GetReturnValue().Set(result);
}

// ════════════════════════════════════════════════════════════════════════════
// Mask callbacks
// ════════════════════════════════════════════════════════════════════════════

void CanvasApplyMaskCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    if (args.Length() < 1) return;
    std::vector<uint8_t> mask;
    if (!Uint8ArrayFromValue(ctx, args[0], &mask)) {
        // Try reading as canvas pixels (luma)
        if (args[0]->IsObject()) {
            int mw = 0, mh = 0; v8::Local<v8::Array> mpix;
            if (ReadCanvasData(ctx, args[0].As<v8::Object>(), &mw, &mh, &mpix)) {
                mask.resize(static_cast<size_t>(mw * mh));
                for (size_t i = 0; i < mask.size(); ++i) {
                    v8::Local<v8::Value> pv;
                    if (!mpix->Get(ctx, static_cast<uint32_t>(i)).ToLocal(&pv) || !pv->IsNumber()) continue;
                    const uint32_t px = static_cast<uint32_t>(pv->NumberValue(ctx).FromMaybe(0.0));
                    const uint8_t r = (px >> 24) & 0xFF, g = (px >> 16) & 0xFF, bv = (px >> 8) & 0xFF;
                    mask[i] = static_cast<uint8_t>(0.2126f * r + 0.7152f * g + 0.0722f * bv);
                }
            }
        }
    }
    if (mask.empty()) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::ApplyMask(&v, w, h, mask);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasFromColorRangeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> src;
    if (!PixelArrayToVector(ctx, pixels, w, h, &src)) return;
    std::vector<uint8_t> mask;
    engine::bridge::skia::MaskFromColorRange(src, w, h,
        ReadU32(args,0,0u), ReadInt(args,1,32), &mask);
    // Return Uint8Array
    const size_t len = mask.size();
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, len);
    if (len > 0) std::memcpy(ab->GetBackingStore()->Data(), mask.data(), len);
    args.GetReturnValue().Set(v8::Uint8Array::New(ab, 0, len));
}

// ════════════════════════════════════════════════════════════════════════════
// Histogram callbacks
// ════════════════════════════════════════════════════════════════════════════

void CanvasHistogramCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) {
        args.GetReturnValue().Set(v8::Null(isolate));
        return;
    }
    engine::bridge::skia::Histogram hist;
    engine::bridge::skia::ComputeHistogram(v, w, h, &hist);
    v8::Local<v8::Object> result = v8::Object::New(isolate);
    auto fill_arr = [&](const char* key, const std::vector<uint32_t>& data) {
        v8::Local<v8::Array> arr = v8::Array::New(isolate, 256);
        for (int i = 0; i < 256; ++i) {
            (void)arr->Set(ctx, static_cast<uint32_t>(i),
                           v8::Number::New(isolate, data[static_cast<size_t>(i)])).FromMaybe(false);
        }
        (void)result->Set(ctx, v8::String::NewFromUtf8(isolate, key).ToLocalChecked(), arr).FromMaybe(false);
    };
    fill_arr("r",    hist.r);
    fill_arr("g",    hist.g);
    fill_arr("b",    hist.b);
    fill_arr("a",    hist.a);
    fill_arr("luma", hist.luma);
    args.GetReturnValue().Set(result);
}

void CanvasAutoLevelsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::AutoLevels(&v, w, h, ReadDbl(args, 0, 0.1));
    PixelVectorToArray(isolate, ctx, v, pixels);
}

void CanvasEqualizeHistogramCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    int w = 0, h = 0; v8::Local<v8::Array> pixels;
    if (!ReadCanvasData(ctx, args.This(), &w, &h, &pixels)) return;
    std::vector<uint32_t> v;
    if (!PixelArrayToVector(ctx, pixels, w, h, &v)) return;
    engine::bridge::skia::EqualizeHistogram(&v, w, h);
    PixelVectorToArray(isolate, ctx, v, pixels);
}

// ════════════════════════════════════════════════════════════════════════════
// Bitmap class
// ════════════════════════════════════════════════════════════════════════════
// Bitmap is structurally identical to Canvas (same _pixels/_width/_height)
// but is a separate constructor exposed as Skia.Bitmap.

void BitmapConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Re-use the same construction logic as CanvasConstructor
    CanvasConstructor(args);
}

// toImage: return an Image object backed by same pixels
void BitmapToImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // Same as CanvasToImageCallback (Canvas::toImage)
    CanvasToImageCallback(args);
}

// toCanvas: return a Canvas backed by copied pixels
void BitmapToCanvasCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CanvasCloneCallback(args);  // clone produces new Canvas via GetSkiaConstructor("Canvas")
}

// toSurface: wrap in Surface
void BitmapToSurfaceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CanvasToSurfaceCallback(args);
}

}  // namespace

namespace modules {

bool RegisterSkiaModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> module = v8::Object::New(isolate);
    bool ok = module
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "available"),
                        v8::Boolean::New(isolate, engine::bridge::skia::IsAvailable()))
                  .FromMaybe(false);

        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "nativeInclude"),
                     v8::Boolean::New(isolate, engine::bridge::skia::HasNativeIncludes()))
                 .FromMaybe(false);

        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "nativeRaster"),
                     v8::Boolean::New(isolate, engine::bridge::skia::HasNativeRaster()))
                 .FromMaybe(false);

    v8::Local<v8::String> summary =
        v8::String::NewFromUtf8(isolate, engine::bridge::skia::Summary().c_str())
            .ToLocalChecked();
    ok = ok && module
                   ->Set(context, v8::String::NewFromUtf8Literal(isolate, "summary"), summary)
                   .FromMaybe(false);

        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "version"),
                     v8::Function::New(context, VersionCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "functions"),
                     v8::Function::New(context, FunctionsCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "blendModes"),
                     v8::Function::New(context, BlendModesCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "hasFunction"),
                     v8::Function::New(context, HasFunctionCallback).ToLocalChecked())
                 .FromMaybe(false);

        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "degToRad"),
                     v8::Function::New(context, DegToRadCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "radToDeg"),
                     v8::Function::New(context, RadToDegCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "clamp01"),
                     v8::Function::New(context, Clamp01Callback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "lerp"),
                     v8::Function::New(context, LerpCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "mapRange"),
                     v8::Function::New(context, MapRangeCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "distance"),
                     v8::Function::New(context, DistanceCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "pointToString"),
                     v8::Function::New(context, PointToStringCallback).ToLocalChecked())
                 .FromMaybe(false);

        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "makeColor"),
                     v8::Function::New(context, MakeColorCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "colorToHex"),
                     v8::Function::New(context, ColorToHexCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "parseColorHex"),
                     v8::Function::New(context, ParseColorHexCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "premultiplyAlpha"),
                     v8::Function::New(context, PremultiplyAlphaCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "unpremultiplyAlpha"),
                     v8::Function::New(context, UnpremultiplyAlphaCallback).ToLocalChecked())
                 .FromMaybe(false);

        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "normalizeBlendMode"),
                     v8::Function::New(context, NormalizeBlendModeCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "blendSrcOver"),
                     v8::Function::New(context, BlendSrcOverCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "blend"),
                     v8::Function::New(context, BlendCallback).ToLocalChecked())
                 .FromMaybe(false);

        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "matrixIdentity"),
                     v8::Function::New(context, MatrixIdentityCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "matrixTranslate"),
                     v8::Function::New(context, MatrixTranslateCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "matrixScale"),
                     v8::Function::New(context, MatrixScaleCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "matrixRotate"),
                     v8::Function::New(context, MatrixRotateCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "matrixMultiply"),
                     v8::Function::New(context, MatrixMultiplyCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "matrixInvert"),
                     v8::Function::New(context, MatrixInvertCallback).ToLocalChecked())
                 .FromMaybe(false);

            v8::Local<v8::FunctionTemplate> canvas_tpl = v8::FunctionTemplate::New(isolate, CanvasConstructor);
            canvas_tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Canvas"));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "clear",
                                                 v8::FunctionTemplate::New(isolate, CanvasClearCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "drawRect",
                                                 v8::FunctionTemplate::New(isolate, CanvasDrawRectCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "getPixel",
                                                 v8::FunctionTemplate::New(isolate, CanvasGetPixelCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "setPixel",
                                                 v8::FunctionTemplate::New(isolate, CanvasSetPixelCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "toImage",
                                                 v8::FunctionTemplate::New(isolate, CanvasToImageCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "pixels",
                                                 v8::FunctionTemplate::New(isolate, CanvasPixelsCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "clone",
                                                 v8::FunctionTemplate::New(isolate, CanvasCloneCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "resize",
                                                 v8::FunctionTemplate::New(isolate, CanvasResizeCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "toSurface",
                                                 v8::FunctionTemplate::New(isolate, CanvasToSurfaceCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "drawLine",
                                                 v8::FunctionTemplate::New(isolate, CanvasDrawLineCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "drawCircle",
                                                 v8::FunctionTemplate::New(isolate, CanvasDrawCircleCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "drawImage",
                                                 v8::FunctionTemplate::New(isolate, CanvasDrawImageCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "drawTriangle",
                                                 v8::FunctionTemplate::New(isolate, CanvasDrawTriangleCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "drawPolyline",
                                                 v8::FunctionTemplate::New(isolate, CanvasDrawPolylineCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "fillPolygon",
                                                 v8::FunctionTemplate::New(isolate, CanvasFillPolygonCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "drawPath",
                                                 v8::FunctionTemplate::New(isolate, CanvasDrawPathCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "clipRect",
                                                 v8::FunctionTemplate::New(isolate, CanvasClipRectCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "clipRestore",
                                                 v8::FunctionTemplate::New(isolate, CanvasClipRestoreCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "save",
                                                 v8::FunctionTemplate::New(isolate, CanvasSaveCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "restore",
                                                 v8::FunctionTemplate::New(isolate, CanvasRestoreCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "encodePNG",
                                                 v8::FunctionTemplate::New(isolate, CanvasEncodePNGCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "encodeJPEG",
                                                 v8::FunctionTemplate::New(isolate, CanvasEncodeJPEGCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "width",
                                                 v8::FunctionTemplate::New(isolate, CanvasWidthCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate,
                                     "height",
                                                 v8::FunctionTemplate::New(isolate, CanvasHeightCallback));
            // ── Photoshop-level transforms ────────────────────────────────────────
            canvas_tpl->PrototypeTemplate()->Set(isolate, "scale",      v8::FunctionTemplate::New(isolate, CanvasScaleCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "rotate",     v8::FunctionTemplate::New(isolate, CanvasRotateCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "flipH",      v8::FunctionTemplate::New(isolate, CanvasFlipHCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "flipV",      v8::FunctionTemplate::New(isolate, CanvasFlipVCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "crop",       v8::FunctionTemplate::New(isolate, CanvasCropCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "composite",  v8::FunctionTemplate::New(isolate, CanvasCompositeCallback));
            // ── Colour adjustments ────────────────────────────────────────────────
            canvas_tpl->PrototypeTemplate()->Set(isolate, "brightnessContrast", v8::FunctionTemplate::New(isolate, CanvasAdjustBrightnessContrastCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "hsl",         v8::FunctionTemplate::New(isolate, CanvasAdjustHSLCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "exposure",    v8::FunctionTemplate::New(isolate, CanvasAdjustExposureCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "levels",      v8::FunctionTemplate::New(isolate, CanvasAdjustLevelsCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "curves",      v8::FunctionTemplate::New(isolate, CanvasAdjustCurvesCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "colorBalance",v8::FunctionTemplate::New(isolate, CanvasAdjustColorBalanceCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "vibrance",    v8::FunctionTemplate::New(isolate, CanvasAdjustVibranceCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "channelMixer",v8::FunctionTemplate::New(isolate, CanvasAdjustChannelMixerCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "colorReplace",v8::FunctionTemplate::New(isolate, CanvasColorReplaceCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "invert",      v8::FunctionTemplate::New(isolate, CanvasInvertCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "grayscale",   v8::FunctionTemplate::New(isolate, CanvasGrayscaleCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "sepia",       v8::FunctionTemplate::New(isolate, CanvasSepiaCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "threshold",   v8::FunctionTemplate::New(isolate, CanvasThresholdCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "posterize",   v8::FunctionTemplate::New(isolate, CanvasPosterizeCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "opacity",     v8::FunctionTemplate::New(isolate, CanvasOpacityCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "selectiveColor", v8::FunctionTemplate::New(isolate, CanvasSelectiveColorCallback));
            // ── Filters ──────────────────────────────────────────────────────────
            canvas_tpl->PrototypeTemplate()->Set(isolate, "blur",            v8::FunctionTemplate::New(isolate, CanvasFilterBlurCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "gaussianBlur",    v8::FunctionTemplate::New(isolate, CanvasFilterGaussianBlurCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "motionBlur",      v8::FunctionTemplate::New(isolate, CanvasFilterMotionBlurCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "radialBlur",      v8::FunctionTemplate::New(isolate, CanvasFilterRadialBlurCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "sharpen",         v8::FunctionTemplate::New(isolate, CanvasFilterSharpenCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "emboss",          v8::FunctionTemplate::New(isolate, CanvasFilterEmbossCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "edgeDetect",      v8::FunctionTemplate::New(isolate, CanvasFilterEdgeDetectCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "addNoise",        v8::FunctionTemplate::New(isolate, CanvasFilterAddNoiseCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "median",          v8::FunctionTemplate::New(isolate, CanvasFilterMedianCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "pixelate",        v8::FunctionTemplate::New(isolate, CanvasFilterPixelateCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "oilPaint",        v8::FunctionTemplate::New(isolate, CanvasFilterOilPaintCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "vignette",        v8::FunctionTemplate::New(isolate, CanvasFilterVignetteCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "chromaticAberration", v8::FunctionTemplate::New(isolate, CanvasFilterChromaticAberrationCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "convolve",        v8::FunctionTemplate::New(isolate, CanvasFilterConvolveCallback));
            // ── Drawing enhancements ──────────────────────────────────────────────
            canvas_tpl->PrototypeTemplate()->Set(isolate, "linearGradient",  v8::FunctionTemplate::New(isolate, CanvasLinearGradientCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "radialGradient",  v8::FunctionTemplate::New(isolate, CanvasRadialGradientCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "sweepGradient",   v8::FunctionTemplate::New(isolate, CanvasSweepGradientCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "floodFill",       v8::FunctionTemplate::New(isolate, CanvasFloodFillCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "drawRoundRect",   v8::FunctionTemplate::New(isolate, CanvasDrawRoundRectCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "drawEllipse",     v8::FunctionTemplate::New(isolate, CanvasDrawEllipseCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "drawArc",         v8::FunctionTemplate::New(isolate, CanvasDrawArcCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "drawThickLine",   v8::FunctionTemplate::New(isolate, CanvasDrawThickLineCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "dropShadow",      v8::FunctionTemplate::New(isolate, CanvasDropShadowCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "glow",            v8::FunctionTemplate::New(isolate, CanvasGlowCallback));
            // ── Text ─────────────────────────────────────────────────────────────
            canvas_tpl->PrototypeTemplate()->Set(isolate, "drawText",        v8::FunctionTemplate::New(isolate, CanvasDrawTextCallback));
            // ── Mask ─────────────────────────────────────────────────────────────
            canvas_tpl->PrototypeTemplate()->Set(isolate, "applyMask",       v8::FunctionTemplate::New(isolate, CanvasApplyMaskCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "maskFromColor",   v8::FunctionTemplate::New(isolate, CanvasFromColorRangeCallback));
            // ── Histogram ────────────────────────────────────────────────────────
            canvas_tpl->PrototypeTemplate()->Set(isolate, "histogram",       v8::FunctionTemplate::New(isolate, CanvasHistogramCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "autoLevels",      v8::FunctionTemplate::New(isolate, CanvasAutoLevelsCallback));
            canvas_tpl->PrototypeTemplate()->Set(isolate, "equalizeHistogram", v8::FunctionTemplate::New(isolate, CanvasEqualizeHistogramCallback));
            v8::Local<v8::Function> canvas_ctor = canvas_tpl->GetFunction(context).ToLocalChecked();
            ok = ok && module->Set(context, v8::String::NewFromUtf8Literal(isolate, "Canvas"), canvas_ctor).FromMaybe(false);

            v8::Local<v8::FunctionTemplate> image_tpl = v8::FunctionTemplate::New(isolate, ImageConstructor);
            image_tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Image"));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "getPixel",
                                                v8::FunctionTemplate::New(isolate, ImageGetPixelCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "setPixel",
                                                v8::FunctionTemplate::New(isolate, ImageSetPixelCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "clone",
                                                v8::FunctionTemplate::New(isolate, ImageCloneCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "toCanvas",
                                                v8::FunctionTemplate::New(isolate, ImageToCanvasCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "pixels",
                                                v8::FunctionTemplate::New(isolate, ImagePixelsCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "width",
                                                v8::FunctionTemplate::New(isolate, ImageWidthCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "height",
                                                v8::FunctionTemplate::New(isolate, ImageHeightCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "toSurface",
                                                v8::FunctionTemplate::New(isolate, ImageToSurfaceCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "encodePNG",
                                                v8::FunctionTemplate::New(isolate, ImageEncodePNGCallback));
            image_tpl->PrototypeTemplate()->Set(isolate,
                                    "encodeJPEG",
                                                v8::FunctionTemplate::New(isolate, ImageEncodeJPEGCallback));
            // ── Photoshop methods (re-use Canvas callbacks since Image and Canvas share same layout)
            image_tpl->PrototypeTemplate()->Set(isolate, "scale",      v8::FunctionTemplate::New(isolate, CanvasScaleCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "rotate",     v8::FunctionTemplate::New(isolate, CanvasRotateCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "flipH",      v8::FunctionTemplate::New(isolate, CanvasFlipHCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "flipV",      v8::FunctionTemplate::New(isolate, CanvasFlipVCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "crop",       v8::FunctionTemplate::New(isolate, CanvasCropCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "composite",  v8::FunctionTemplate::New(isolate, CanvasCompositeCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "brightnessContrast", v8::FunctionTemplate::New(isolate, CanvasAdjustBrightnessContrastCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "hsl",        v8::FunctionTemplate::New(isolate, CanvasAdjustHSLCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "exposure",   v8::FunctionTemplate::New(isolate, CanvasAdjustExposureCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "levels",     v8::FunctionTemplate::New(isolate, CanvasAdjustLevelsCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "curves",     v8::FunctionTemplate::New(isolate, CanvasAdjustCurvesCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "colorBalance",v8::FunctionTemplate::New(isolate, CanvasAdjustColorBalanceCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "vibrance",   v8::FunctionTemplate::New(isolate, CanvasAdjustVibranceCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "channelMixer",v8::FunctionTemplate::New(isolate, CanvasAdjustChannelMixerCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "colorReplace",v8::FunctionTemplate::New(isolate, CanvasColorReplaceCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "invert",     v8::FunctionTemplate::New(isolate, CanvasInvertCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "grayscale",  v8::FunctionTemplate::New(isolate, CanvasGrayscaleCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "sepia",      v8::FunctionTemplate::New(isolate, CanvasSepiaCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "threshold",  v8::FunctionTemplate::New(isolate, CanvasThresholdCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "posterize",  v8::FunctionTemplate::New(isolate, CanvasPosterizeCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "opacity",    v8::FunctionTemplate::New(isolate, CanvasOpacityCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "blur",       v8::FunctionTemplate::New(isolate, CanvasFilterBlurCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "gaussianBlur",v8::FunctionTemplate::New(isolate, CanvasFilterGaussianBlurCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "sharpen",    v8::FunctionTemplate::New(isolate, CanvasFilterSharpenCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "emboss",     v8::FunctionTemplate::New(isolate, CanvasFilterEmbossCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "edgeDetect", v8::FunctionTemplate::New(isolate, CanvasFilterEdgeDetectCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "addNoise",   v8::FunctionTemplate::New(isolate, CanvasFilterAddNoiseCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "pixelate",   v8::FunctionTemplate::New(isolate, CanvasFilterPixelateCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "vignette",   v8::FunctionTemplate::New(isolate, CanvasFilterVignetteCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "histogram",  v8::FunctionTemplate::New(isolate, CanvasHistogramCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "autoLevels", v8::FunctionTemplate::New(isolate, CanvasAutoLevelsCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "equalizeHistogram", v8::FunctionTemplate::New(isolate, CanvasEqualizeHistogramCallback));
            image_tpl->PrototypeTemplate()->Set(isolate, "applyMask",  v8::FunctionTemplate::New(isolate, CanvasApplyMaskCallback));
            v8::Local<v8::Function> image_ctor = image_tpl->GetFunction(context).ToLocalChecked();
            ok = ok && module->Set(context, v8::String::NewFromUtf8Literal(isolate, "Image"), image_ctor).FromMaybe(false);

            v8::Local<v8::FunctionTemplate> surface_tpl = v8::FunctionTemplate::New(isolate, SurfaceConstructor);
            surface_tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Surface"));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "getCanvas",
                                                  v8::FunctionTemplate::New(isolate, SurfaceGetCanvasCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "clear",
                                                  v8::FunctionTemplate::New(isolate, SurfaceClearCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "drawRect",
                                                  v8::FunctionTemplate::New(isolate, SurfaceDrawRectCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "drawLine",
                                                  v8::FunctionTemplate::New(isolate, SurfaceDrawLineCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "drawCircle",
                                                  v8::FunctionTemplate::New(isolate, SurfaceDrawCircleCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "drawTriangle",
                                                  v8::FunctionTemplate::New(isolate, SurfaceDrawTriangleCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "drawPolyline",
                                                  v8::FunctionTemplate::New(isolate, SurfaceDrawPolylineCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "fillPolygon",
                                                  v8::FunctionTemplate::New(isolate, SurfaceFillPolygonCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "drawImage",
                                                  v8::FunctionTemplate::New(isolate, SurfaceDrawImageCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "makeImageSnapshot",
                                                  v8::FunctionTemplate::New(isolate, SurfaceMakeImageSnapshotCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "toImage",
                                                  v8::FunctionTemplate::New(isolate, SurfaceToImageCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "resize",
                                                  v8::FunctionTemplate::New(isolate, SurfaceResizeCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "flush",
                                                  v8::FunctionTemplate::New(isolate, SurfaceFlushCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "width",
                                                  v8::FunctionTemplate::New(isolate, SurfaceWidthCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "height",
                                                  v8::FunctionTemplate::New(isolate, SurfaceHeightCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "drawPath",
                                                  v8::FunctionTemplate::New(isolate, SurfaceDrawPathCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "clipRect",
                                                  v8::FunctionTemplate::New(isolate, SurfaceClipRectCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "clipRestore",
                                                  v8::FunctionTemplate::New(isolate, SurfaceClipRestoreCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "save",
                                                  v8::FunctionTemplate::New(isolate, SurfaceSaveCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "restore",
                                                  v8::FunctionTemplate::New(isolate, SurfaceRestoreCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "encodePNG",
                                                  v8::FunctionTemplate::New(isolate, SurfaceEncodePNGCallback));
            surface_tpl->PrototypeTemplate()->Set(isolate,
                                    "encodeJPEG",
                                                  v8::FunctionTemplate::New(isolate, SurfaceEncodeJPEGCallback));
            v8::Local<v8::Function> surface_ctor = surface_tpl->GetFunction(context).ToLocalChecked();
            ok = ok && module->Set(context, v8::String::NewFromUtf8Literal(isolate, "Surface"), surface_ctor).FromMaybe(false);

            // ─── Path ────────────────────────────────────────────────────────────
            v8::Local<v8::FunctionTemplate> path_tpl = v8::FunctionTemplate::New(isolate, PathConstructor);
            path_tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Path"));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "moveTo", v8::FunctionTemplate::New(isolate, PathMoveToCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "lineTo", v8::FunctionTemplate::New(isolate, PathLineToCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "quadTo", v8::FunctionTemplate::New(isolate, PathQuadToCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "conicTo", v8::FunctionTemplate::New(isolate, PathConicToCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "cubicTo", v8::FunctionTemplate::New(isolate, PathCubicToCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "arcTo", v8::FunctionTemplate::New(isolate, PathArcToCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "close", v8::FunctionTemplate::New(isolate, PathCloseCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "reset", v8::FunctionTemplate::New(isolate, PathResetCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "isEmpty", v8::FunctionTemplate::New(isolate, PathIsEmptyCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "cmds", v8::FunctionTemplate::New(isolate, PathCmdsCallback));
            path_tpl->PrototypeTemplate()->Set(isolate,
                                    "clone", v8::FunctionTemplate::New(isolate, PathCloneCallback));
            v8::Local<v8::Function> path_ctor = path_tpl->GetFunction(context).ToLocalChecked();
            ok = ok && module->Set(context, v8::String::NewFromUtf8Literal(isolate, "Path"), path_ctor).FromMaybe(false);

            // ─── decodeImage ──────────────────────────────────────────────────────
            ok = ok && module
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "decodeImage"),
                         v8::Function::New(context, DecodeImageCallback).ToLocalChecked())
                     .FromMaybe(false);

            v8::Local<v8::FunctionTemplate> paint_tpl = v8::FunctionTemplate::New(isolate, PaintConstructor);
            paint_tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Paint"));
            v8::Local<v8::Function> paint_ctor = paint_tpl->GetFunction(context).ToLocalChecked();
            ok = ok && module->Set(context, v8::String::NewFromUtf8Literal(isolate, "Paint"), paint_ctor).FromMaybe(false);

            // ─── Bitmap ─────────────────────────────────────────────────────────
            v8::Local<v8::FunctionTemplate> bitmap_tpl = v8::FunctionTemplate::New(isolate, BitmapConstructor);
            bitmap_tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Bitmap"));
            // Same pixel API as Canvas
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "clear",     v8::FunctionTemplate::New(isolate, CanvasClearCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "getPixel",  v8::FunctionTemplate::New(isolate, CanvasGetPixelCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "setPixel",  v8::FunctionTemplate::New(isolate, CanvasSetPixelCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "pixels",    v8::FunctionTemplate::New(isolate, CanvasPixelsCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "clone",     v8::FunctionTemplate::New(isolate, CanvasCloneCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "width",     v8::FunctionTemplate::New(isolate, CanvasWidthCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "height",    v8::FunctionTemplate::New(isolate, CanvasHeightCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "toImage",   v8::FunctionTemplate::New(isolate, BitmapToImageCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "toCanvas",  v8::FunctionTemplate::New(isolate, BitmapToCanvasCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "toSurface", v8::FunctionTemplate::New(isolate, BitmapToSurfaceCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "encodePNG", v8::FunctionTemplate::New(isolate, CanvasEncodePNGCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "encodeJPEG",v8::FunctionTemplate::New(isolate, CanvasEncodeJPEGCallback));
            // Drawing
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawRect",      v8::FunctionTemplate::New(isolate, CanvasDrawRectCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawLine",      v8::FunctionTemplate::New(isolate, CanvasDrawLineCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawCircle",    v8::FunctionTemplate::New(isolate, CanvasDrawCircleCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawImage",     v8::FunctionTemplate::New(isolate, CanvasDrawImageCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawPath",      v8::FunctionTemplate::New(isolate, CanvasDrawPathCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawRoundRect", v8::FunctionTemplate::New(isolate, CanvasDrawRoundRectCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawEllipse",   v8::FunctionTemplate::New(isolate, CanvasDrawEllipseCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawArc",       v8::FunctionTemplate::New(isolate, CanvasDrawArcCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawThickLine", v8::FunctionTemplate::New(isolate, CanvasDrawThickLineCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawText",      v8::FunctionTemplate::New(isolate, CanvasDrawTextCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawTriangle",  v8::FunctionTemplate::New(isolate, CanvasDrawTriangleCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "fillPolygon",   v8::FunctionTemplate::New(isolate, CanvasFillPolygonCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "drawPolyline",  v8::FunctionTemplate::New(isolate, CanvasDrawPolylineCallback));
            // Transforms
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "scale",        v8::FunctionTemplate::New(isolate, CanvasScaleCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "rotate",       v8::FunctionTemplate::New(isolate, CanvasRotateCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "flipH",        v8::FunctionTemplate::New(isolate, CanvasFlipHCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "flipV",        v8::FunctionTemplate::New(isolate, CanvasFlipVCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "crop",         v8::FunctionTemplate::New(isolate, CanvasCropCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "composite",    v8::FunctionTemplate::New(isolate, CanvasCompositeCallback));
            // Adjustments
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "brightnessContrast", v8::FunctionTemplate::New(isolate, CanvasAdjustBrightnessContrastCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "hsl",           v8::FunctionTemplate::New(isolate, CanvasAdjustHSLCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "exposure",      v8::FunctionTemplate::New(isolate, CanvasAdjustExposureCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "levels",        v8::FunctionTemplate::New(isolate, CanvasAdjustLevelsCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "curves",        v8::FunctionTemplate::New(isolate, CanvasAdjustCurvesCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "colorBalance",  v8::FunctionTemplate::New(isolate, CanvasAdjustColorBalanceCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "vibrance",      v8::FunctionTemplate::New(isolate, CanvasAdjustVibranceCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "channelMixer",  v8::FunctionTemplate::New(isolate, CanvasAdjustChannelMixerCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "colorReplace",  v8::FunctionTemplate::New(isolate, CanvasColorReplaceCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "invert",        v8::FunctionTemplate::New(isolate, CanvasInvertCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "grayscale",     v8::FunctionTemplate::New(isolate, CanvasGrayscaleCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "sepia",         v8::FunctionTemplate::New(isolate, CanvasSepiaCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "threshold",     v8::FunctionTemplate::New(isolate, CanvasThresholdCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "posterize",     v8::FunctionTemplate::New(isolate, CanvasPosterizeCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "opacity",       v8::FunctionTemplate::New(isolate, CanvasOpacityCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "selectiveColor",v8::FunctionTemplate::New(isolate, CanvasSelectiveColorCallback));
            // Filters
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "blur",          v8::FunctionTemplate::New(isolate, CanvasFilterBlurCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "gaussianBlur",  v8::FunctionTemplate::New(isolate, CanvasFilterGaussianBlurCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "motionBlur",    v8::FunctionTemplate::New(isolate, CanvasFilterMotionBlurCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "radialBlur",    v8::FunctionTemplate::New(isolate, CanvasFilterRadialBlurCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "sharpen",       v8::FunctionTemplate::New(isolate, CanvasFilterSharpenCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "emboss",        v8::FunctionTemplate::New(isolate, CanvasFilterEmbossCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "edgeDetect",    v8::FunctionTemplate::New(isolate, CanvasFilterEdgeDetectCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "addNoise",      v8::FunctionTemplate::New(isolate, CanvasFilterAddNoiseCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "median",        v8::FunctionTemplate::New(isolate, CanvasFilterMedianCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "pixelate",      v8::FunctionTemplate::New(isolate, CanvasFilterPixelateCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "oilPaint",      v8::FunctionTemplate::New(isolate, CanvasFilterOilPaintCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "vignette",      v8::FunctionTemplate::New(isolate, CanvasFilterVignetteCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "chromaticAberration", v8::FunctionTemplate::New(isolate, CanvasFilterChromaticAberrationCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "convolve",      v8::FunctionTemplate::New(isolate, CanvasFilterConvolveCallback));
            // Gradients / fill
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "linearGradient",v8::FunctionTemplate::New(isolate, CanvasLinearGradientCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "radialGradient",v8::FunctionTemplate::New(isolate, CanvasRadialGradientCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "sweepGradient", v8::FunctionTemplate::New(isolate, CanvasSweepGradientCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "floodFill",     v8::FunctionTemplate::New(isolate, CanvasFloodFillCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "dropShadow",    v8::FunctionTemplate::New(isolate, CanvasDropShadowCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "glow",          v8::FunctionTemplate::New(isolate, CanvasGlowCallback));
            // Mask / Histogram
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "applyMask",     v8::FunctionTemplate::New(isolate, CanvasApplyMaskCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "maskFromColor", v8::FunctionTemplate::New(isolate, CanvasFromColorRangeCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "histogram",     v8::FunctionTemplate::New(isolate, CanvasHistogramCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "autoLevels",    v8::FunctionTemplate::New(isolate, CanvasAutoLevelsCallback));
            bitmap_tpl->PrototypeTemplate()->Set(isolate, "equalizeHistogram", v8::FunctionTemplate::New(isolate, CanvasEqualizeHistogramCallback));
            v8::Local<v8::Function> bitmap_ctor = bitmap_tpl->GetFunction(context).ToLocalChecked();
            ok = ok && module->Set(context, v8::String::NewFromUtf8Literal(isolate, "Bitmap"), bitmap_ctor).FromMaybe(false);

            // ─── measureText ─────────────────────────────────────────────────────
            ok = ok && module
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "measureText"),
                         v8::Function::New(context, SkiaMeasureTextCallback).ToLocalChecked())
                     .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Skia"), module)
        .FromMaybe(false);
}

}  // namespace modules