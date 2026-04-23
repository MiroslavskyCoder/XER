#include "modules/module_registry.h"

#include "modules/bcrypto_module.h"
#include "modules/container_module.h"
#include "modules/brotli_module.h"
#include "modules/crypto_module.h"
#include "modules/console_module.h"
#include "modules/buffer_module.h"
#include "modules/bridge/cuda_module.h"
#include "modules/bridge/cudnn_module.h"
#include "modules/filesystem_module.h"
#include "modules/bridge/ffmpeg_module.h"
#include "modules/git_module.h"
#include "modules/io_async_module.h"
#include "modules/io_formating_module.h"
#include "modules/network_module.h"
#include "modules/bridge/opencv_module.h"
#include "modules/provider_module.h"
#include "modules/qt6/qt_module.h"
#include "modules/qt6/qt_core_module.h"
#include "modules/qt6/qt_gui_module.h"
#include "modules/qt6/qt_xml_module.h"
#include "modules/qt6/qt_pdf_module.h"
#include "modules/qt6/qt_web_module.h"
#include "modules/qt6/qt_qml_module.h"
#include "modules/qt6/qt_quick_module.h"
#include "modules/runtime_live_module.h"
#include "modules/system_module.h"
#include "modules/timer_module.h"
#include "modules/sqlite_module.h"
#include "modules/util_module.h"
#include "modules/bridge/angle_module.h"
#include "modules/bridge/skia_module.h"
#include "modules/zlib_module.h"
#include "modules/zstd_module.h"

namespace modules {

bool ImportModule(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  const std::string& module_name,
                  std::string* error_message) {
    if (module_name == "FileSystem") {
        return RegisterFileSystemModule(isolate, context);
    }

    if (module_name == "RuntimeLive") {
        return RegisterRuntimeLiveModule(isolate, context);
    }

    if (module_name == "Provider") {
        return RegisterProviderModule(isolate, context);
    }

    if (module_name == "OpenCV") {
        return RegisterOpenCVModule(isolate, context);
    }

    if (module_name == "CUDA") {
        return RegisterCUDAModule(isolate, context);
    }

    if (module_name == "CUDNN") {
        return RegisterCUDNNModule(isolate, context);
    }

    if (module_name == "Skia") {
        return RegisterSkiaModule(isolate, context);
    }

    if (module_name == "FFmpeg") {
        return RegisterFFmpegModule(isolate, context);
    }

    if (module_name == "ANGLE") {
        return RegisterANGLEModule(isolate, context);
    }

    if (module_name == "Qt") {
        return RegisterQtModule(isolate, context);
    }
    if (module_name == "Qt/All") {
        return RegisterQtModule(isolate, context)
            && RegisterQtCoreModule(isolate, context)
            && RegisterQtGuiModule(isolate, context)
            && RegisterQtXmlModule(isolate, context)
            && RegisterQtPdfModule(isolate, context)
            && RegisterQtWebModule(isolate, context)
            && RegisterQtQmlModule(isolate, context)
            && RegisterQtQuickModule(isolate, context);
    }
    if (module_name == "Qt/Core") {
        return RegisterQtCoreModule(isolate, context);
    }
    if (module_name == "Qt/Gui") {
        return RegisterQtGuiModule(isolate, context);
    }
    if (module_name == "Qt/Widgets") {
        return RegisterQtGuiModule(isolate, context);
    }
    if (module_name == "Qt/Application") {
        return RegisterQtGuiModule(isolate, context);
    }
    if (module_name == "Qt/Xml") {
        return RegisterQtXmlModule(isolate, context);
    }
    if (module_name == "Qt/Pdf") {
        return RegisterQtPdfModule(isolate, context);
    }
    if (module_name == "Qt/Web") {
        return RegisterQtWebModule(isolate, context);
    }
    if (module_name == "Qt/WebEngine") {
        return RegisterQtWebModule(isolate, context);
    }
    if (module_name == "Qt/WebView") {
        return RegisterQtWebModule(isolate, context);
    }
    if (module_name == "Qt/WebChannel") {
        return RegisterQtWebModule(isolate, context);
    }
    if (module_name == "Qt/Qml") {
        return RegisterQtQmlModule(isolate, context);
    }
    if (module_name == "Qt/Quick") {
        return RegisterQtQuickModule(isolate, context);
    }

    if (module_name == "Container") {
        return RegisterContainerModule(isolate, context);
    }

    if (module_name == "Util") {
        return RegisterUtilModule(isolate, context);
    }

    if (module_name == "Crypto") {
        return RegisterCryptoModule(isolate, context);
    }

    if (module_name == "Network") {
        return RegisterNetworkModule(isolate, context);
    }

    if (module_name == "System") {
        return RegisterSystemModule(isolate, context);
    }

    if (module_name == "Console") {
        return RegisterConsoleModule(isolate, context);
    }

    if (module_name == "Buffer") {
        return RegisterBufferModule(isolate, context);
    }

    if (module_name == "Git") {
        return RegisterGitModule(isolate, context);
    }

    if (module_name == "Zlib") {
        return RegisterZlibModule(isolate, context);
    }

    if (module_name == "Zstd") {
        return RegisterZstdModule(isolate, context);
    }

    if (module_name == "Brotli") {
        return RegisterBrotliModule(isolate, context);
    }

    if (module_name == "Bcrypto") {
        return RegisterBcryptoModule(isolate, context);
    }

    if (module_name == "Timer") {
        return RegisterTimerModule(isolate, context);
    }

    if (module_name == "SQLite") {
        return RegisterSQLiteModule(isolate, context);
    }

    if (module_name == "IO/Async") {
        return RegisterIOAsyncModule(isolate, context);
    }

    if (module_name == "IO/Formating") {
        return RegisterIOFormatingModule(isolate, context);
    }

    if (error_message != nullptr) {
        *error_message = "Unknown module: " + module_name;
    }
    return false;
}

}  // namespace modules
