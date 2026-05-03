#include "cuda_reduce_ops.h"

#include <cuda_runtime.h>

namespace Engine::ML::CudaOps {

__global__ void kernel_reduce_sum(const float* input, float* output, size_t n) {
    extern __shared__ float sdata[];
    size_t tid = threadIdx.x;
    size_t idx = blockIdx.x * blockDim.x + tid;
    sdata[tid] = (idx < n) ? input[idx] : 0.0f;
    __syncthreads();
    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) sdata[tid] += sdata[tid + s];
        __syncthreads();
    }
    if (tid == 0) atomicAdd(output, sdata[0]);
}

__global__ void kernel_reduce_max(const float* input, float* output, size_t n) {
    extern __shared__ float sdata[];
    size_t tid = threadIdx.x;
    size_t idx = blockIdx.x * blockDim.x + tid;
    sdata[tid] = (idx < n) ? input[idx] : -3.402823466e+38f;
    __syncthreads();
    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s && sdata[tid + s] > sdata[tid]) sdata[tid] = sdata[tid + s];
        __syncthreads();
    }
    if (tid == 0) atomicMax((int*)output, __float_as_int(sdata[0]));
}

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
