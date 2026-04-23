#include "wrapper/qt6/image/image_v8.h"
#include "wrapper/qt6/image/image.h"
#include "wrapper/qt6/util/v8_string.h"
#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/v8/module_builder.h"

namespace {

using namespace qt6::v8bridge;

void ThrowLocalTypeError(v8::Isolate* iso, const char* message) {
    iso->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8(iso, message).ToLocalChecked()));
}

void ImageInfoCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowLocalTypeError(iso, "QtImage.info(path: string)");
        return;
    }
    auto info = qt6::image::GetInfo(qt6::util::V8ValueToStdString(iso, args[0]));
    auto obj  = v8::Object::New(iso);
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "valid"),
             v8::Boolean::New(iso, info.valid)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "width"),
             v8::Integer::New(iso, info.width)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "height"),
             v8::Integer::New(iso, info.height)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "depth"),
             v8::Integer::New(iso, info.depth)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "format"),
             v8::String::NewFromUtf8(iso, info.format.c_str()).ToLocalChecked()).Check();
    args.GetReturnValue().Set(obj);
}

void ImageResizeCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 4) {
        ThrowLocalTypeError(iso, "QtImage.resize(src, dst, width, height, keepAspect?)");
        return;
    }
    auto src  = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst  = qt6::util::V8ValueToStdString(iso, args[1]);
    int  w    = args[2]->Int32Value(ctx).FromMaybe(0);
    int  h    = args[3]->Int32Value(ctx).FromMaybe(0);
    bool keep = args.Length() >= 5 ? args[4]->BooleanValue(iso) : true;
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::image::Resize(src, dst, w, h, keep)));
}

void ImageConvertCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        ThrowLocalTypeError(iso, "QtImage.convert(src, dst, quality?)");
        return;
    }
    auto src  = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst  = qt6::util::V8ValueToStdString(iso, args[1]);
    int  qual = args.Length() >= 3 ? args[2]->Int32Value(ctx).FromMaybe(-1) : -1;
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::image::Convert(src, dst, qual)));
}

void ImageCropCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 6) {
        ThrowLocalTypeError(iso, "QtImage.crop(src, dst, x, y, width, height)");
        return;
    }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    int  x   = args[2]->Int32Value(ctx).FromMaybe(0);
    int  y   = args[3]->Int32Value(ctx).FromMaybe(0);
    int  w   = args[4]->Int32Value(ctx).FromMaybe(0);
    int  h   = args[5]->Int32Value(ctx).FromMaybe(0);
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::image::Crop(src, dst, x, y, w, h)));
}

void ImageFlipHCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    if (args.Length() < 2) { ThrowLocalTypeError(iso, "QtImage.flipH(src, dst)"); return; }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::image::FlipH(src, dst)));
}

void ImageFlipVCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    if (args.Length() < 2) { ThrowLocalTypeError(iso, "QtImage.flipV(src, dst)"); return; }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::image::FlipV(src, dst)));
}

void ImageRotateCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 3) {
        ThrowLocalTypeError(iso, "QtImage.rotate(src, dst, degrees)");
        return;
    }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    int  deg = args[2]->Int32Value(ctx).FromMaybe(90);
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::image::Rotate(src, dst, deg)));
}

void ImageToGrayscaleCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    if (args.Length() < 2) { ThrowLocalTypeError(iso, "QtImage.toGrayscale(src, dst)"); return; }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::image::ToGrayscale(src, dst)));
}

// isAvailable() — true if Qt6::Gui is linked
void ImageIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if ENGINE_HAS_QT6
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

}  // namespace

namespace qt6::image {

bool RegisterQtImageModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    auto mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "isAvailable", ImageIsAvailableCb);
    ok = ok && SetMethod(isolate, context, mod, "info",        ImageInfoCb);
    ok = ok && SetMethod(isolate, context, mod, "resize",      ImageResizeCb);
    ok = ok && SetMethod(isolate, context, mod, "convert",     ImageConvertCb);
    ok = ok && SetMethod(isolate, context, mod, "crop",        ImageCropCb);
    ok = ok && SetMethod(isolate, context, mod, "flipH",       ImageFlipHCb);
    ok = ok && SetMethod(isolate, context, mod, "flipV",       ImageFlipVCb);
    ok = ok && SetMethod(isolate, context, mod, "rotate",      ImageRotateCb);
    ok = ok && SetMethod(isolate, context, mod, "toGrayscale", ImageToGrayscaleCb);
    if (!ok) return false;
    return ExportGlobalModule(isolate, context, "QtImage", mod);
}

}  // namespace qt6::image
