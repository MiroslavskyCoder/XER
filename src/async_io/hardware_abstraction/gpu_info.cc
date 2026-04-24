#include "gpu_info.h"
 
#include <cuda_runtime.h> 

namespace AsyncIO::IO::Hardware {

GPUInfoProvider::GPUInfoProvider()
    : current_device_(0), initialized_(false), supports_cuda_(false), 
      supports_opencl_(false), supports_hip_(false) {}

GPUInfoProvider::~GPUInfoProvider() {}

bool GPUInfoProvider::Initialize() {
    DetectGPUs();
    initialized_ = true;
    return gpu_devices_.size() > 0;
}

bool GPUInfoProvider::IsInitialized() const {
    return initialized_;
}

int GPUInfoProvider::GetDeviceCount() const {
    return static_cast<int>(gpu_devices_.size());
}

std::vector<GPUDevice> GPUInfoProvider::GetDevices() const {
    return gpu_devices_;
}

std::string GPUInfoProvider::GetDeviceName(int device_id) const {
    if (device_id >= 0 && device_id < static_cast<int>(gpu_devices_.size())) {
        return gpu_devices_[device_id].name;
    }
    return "";
}

uint64_t GPUInfoProvider::GetTotalMemory(int device_id) const {
    if (device_id >= 0 && device_id < static_cast<int>(gpu_devices_.size())) {
        return gpu_devices_[device_id].total_memory_mb;
    }
    return 0;
}

uint64_t GPUInfoProvider::GetFreeMemory(int device_id) const {
    if (device_id >= 0 && device_id < static_cast<int>(gpu_devices_.size())) {
        return gpu_devices_[device_id].free_memory_mb;
    }
    return 0;
}

bool GPUInfoProvider::SetCurrentDevice(int device_id) {
    if (device_id >= 0 && device_id < static_cast<int>(gpu_devices_.size())) {
        current_device_ = device_id;
        return true;
    }
    return false;
}

int GPUInfoProvider::GetCurrentDevice() const {
    return current_device_;
}

std::string GPUInfoProvider::GetDeviceDescription(int device_id) const {
    if (device_id >= 0 && device_id < static_cast<int>(gpu_devices_.size())) {
        const auto& dev = gpu_devices_[device_id];
        return dev.name + " (" + std::to_string(dev.total_memory_mb) + "MB)";
    }
    return "";
}

void GPUInfoProvider::DetectGPUs() {
    DetectCUDADevices();
}

void GPUInfoProvider::DetectCUDADevices() {
    int device_count = 0;
    if (cudaGetDeviceCount(&device_count) == cudaSuccess) {
        supports_cuda_ = true;
        
        for (int i = 0; i < device_count; ++i) {
            cudaDeviceProp props;
            if (cudaGetDeviceProperties(&props, i) == cudaSuccess) {
                GPUDevice dev;
                dev.device_id = i;
                dev.name = props.name;
                dev.compute_capability = std::to_string(props.major) + "." + 
                                        std::to_string(props.minor);
                dev.total_memory_mb = props.totalGlobalMem / (1024 * 1024);
                dev.free_memory_mb = dev.total_memory_mb;  // Simplified
                dev.is_available = true;
                
                gpu_devices_.push_back(dev);
            }
        }
    }
}

}  // namespace AsyncIO::IO::Hardware
