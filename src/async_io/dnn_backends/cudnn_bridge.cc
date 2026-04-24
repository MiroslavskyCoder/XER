#include "cudnn_bridge.h"
 
#include <cudnn.h> 

namespace AsyncIO::IO::DNNBackends {

CuDnnBridge::CuDnnBridge() : handle_(nullptr), ready_(false) {
}

CuDnnBridge::~CuDnnBridge() {
    Shutdown();
}

bool CuDnnBridge::Initialize() {
    if (ready_) {
        return true;
    }
 
    cudnnHandle_t handle = nullptr;
    const cudnnStatus_t status = cudnnCreate(&handle);
    if (status != CUDNN_STATUS_SUCCESS) {
        lastError_ = cudnnGetErrorString(status);
        return false;
    }

    handle_ = handle;
    ready_ = true;
    lastError_.clear();
    return true; 
}

void CuDnnBridge::Shutdown() { 
    if (handle_ != nullptr) {
        cudnnDestroy(static_cast<cudnnHandle_t>(handle_));
    } 
    handle_ = nullptr;
    ready_ = false;
}

bool CuDnnBridge::IsReady() const {
    return ready_;
}

std::string CuDnnBridge::LastError() const {
    return lastError_;
}

void *CuDnnBridge::Handle() const {
    return handle_;
}

} // namespace AsyncIO::IO::DNNBackends
