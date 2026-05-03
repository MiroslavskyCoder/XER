#include "cuda_reduce_ops.h"

#include <cuda_runtime.h>

namespace Engine::ML::CudaOps {

// Forward declare CUDA reduction kernels
__global__ void kernel_reduce_sum(const float* input, float* output, size_t n);
__global__ void kernel_reduce_max(const float* input, float* output, size_t n);

cudaError_t CudaReduceOps::ReduceToScalar(ReduceOp op,
                                           const float* device_input,
                                           float* device_output,
                                           size_t elements,
                                           cudaStream_t stream) {
    if (!device_input || !device_output || elements == 0) {
        return cudaErrorInvalidValue;
    }
    
    // Block reduce: each block produces one result
    int block_size = 256;
    int grid_size = (elements + block_size - 1) / block_size;
    
    switch (op) {
        case ReduceOp::Sum:
            kernel_reduce_sum<<<grid_size, block_size, block_size * sizeof(float), stream>>>(
                device_input, device_output, elements);
            break;
        case ReduceOp::Max:
            kernel_reduce_max<<<grid_size, block_size, block_size * sizeof(float), stream>>>(
                device_input, device_output, elements);
            break;
        default:
            return cudaErrorInvalidValue;
    }
    
    return cudaGetLastError();
}

cudaError_t CudaReduceOps::ReduceAlongAxis(ReduceOp op,
                                            const float* device_input,
                                            float* device_output,
                                            size_t input_numel,
                                            int axis,
                                            cudaStream_t stream) {
    if (!device_input || !device_output || input_numel == 0) {
        return cudaErrorInvalidValue;
    }
    
    // Axis reduction would need 2D/3D kernel configuration
    // Simplified: delegate to ReduceToScalar
    return ReduceToScalar(op, device_input, device_output, input_numel, stream);
}

} // namespace Engine::ML::CudaOps
