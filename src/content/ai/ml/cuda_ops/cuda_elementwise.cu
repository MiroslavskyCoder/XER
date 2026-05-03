#include "cuda_elementwise.h"

#include <cuda_runtime.h>
#include <math.h>

namespace Engine::ML::CudaOps {

__global__ void kernel_add_float(const float* A, const float* B, float* C, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) C[idx] = A[idx] + B[idx];
}

__global__ void kernel_relu_float(const float* A, float* B, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) B[idx] = A[idx] > 0.0f ? A[idx] : 0.0f;
}

__global__ void kernel_sigmoid_float(const float* A, float* B, size_t n) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) B[idx] = 1.0f / (1.0f + expf(-A[idx]));
}

cudaError_t CudaElementwise::ElementwiseBinary(CudaElementwise::BinaryOp op,
                                       const float* device_A,
                                       const float* device_B,
                                       float* device_C,
                                       size_t elements,
                                       cudaStream_t stream) {
    if (!device_A || !device_B || !device_C || elements == 0) {
        return cudaErrorInvalidValue;
    }
    
    // Dispatch to appropriate kernel
    int block_size = 256;
    int grid_size = (elements + block_size - 1) / block_size;
    
    switch (op) {
        case BinaryOp::Add:
            kernel_add_float<<<grid_size, block_size, 0, stream>>>(
                device_A, device_B, device_C, elements);
            break;
        case BinaryOp::Subtract:
            // kernel_sub_float<<<grid_size, block_size, 0, stream>>>(
            //     device_A, device_B, device_C, elements);
            break;
        case BinaryOp::Multiply:
            // kernel_mul_float<<<...>>>(...);
            break;
        default:
            return cudaErrorInvalidValue;
    }
    
    return cudaGetLastError();
}

cudaError_t CudaElementwise::ElementwiseUnary(CudaElementwise::UnaryOp op,
                                      const float* device_A,
                                      float* device_B,
                                      size_t elements,
                                      cudaStream_t stream) {
    if (!device_A || !device_B || elements == 0) {
        return cudaErrorInvalidValue;
    }
    
    int block_size = 256;
    int grid_size = (elements + block_size - 1) / block_size;
    
    switch (op) {
        case UnaryOp::ReLU:
            kernel_relu_float<<<grid_size, block_size, 0, stream>>>(
                device_A, device_B, elements);
            break;
        case UnaryOp::Sigmoid:
            kernel_sigmoid_float<<<grid_size, block_size, 0, stream>>>(
                device_A, device_B, elements);
            break;
        default:
            return cudaErrorInvalidValue;
    }
    
    return cudaGetLastError();
}

cudaError_t CudaElementwise::ElementwiseBinaryScaled(CudaElementwise::BinaryOp op,
                                                float scale,
                                                const float* device_A,
                                                const float* device_B,
                                                float* device_D,
                                                size_t elements,
                                                cudaStream_t stream) {
    // First perform binary operation
    cudaError_t err = ElementwiseBinary(op, device_A, device_B, device_D, elements, stream);
    if (err != cudaSuccess) {
        return err;
    }
    
    // Then scale result (can be fused into single kernel in optimized version)
    // For now, would need additional scaling kernel
    return cudaSuccess;
}

} // namespace Engine::ML::CudaOps
