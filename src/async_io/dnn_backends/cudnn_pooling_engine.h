#ifndef AITOOLSXPRO_IO_DNN_BACKENDS_CUDNN_POOLING_ENGINE_H
#define AITOOLSXPRO_IO_DNN_BACKENDS_CUDNN_POOLING_ENGINE_H

#include <cstddef>

#include "cudnn_bridge.h"

namespace AIToolsXPro::IO::DNNBackends {

enum class PoolingKind {
    Max,
    Average
};

class CuDnnPoolingEngine {
public:
    CuDnnPoolingEngine();

    bool Configure(PoolingKind kind, int window, int stride, int padding);
    bool Forward(
        const float *deviceInput,
        float *deviceOutput,
        int batch,
        int channels,
        int height,
        int width,
        void *stream = nullptr);

private:
    CuDnnBridge bridge_;
    PoolingKind kind_;
    int window_;
    int stride_;
    int padding_;
    bool configured_;
};

} // namespace AIToolsXPro::IO::DNNBackends

#endif // AITOOLSXPRO_IO_DNN_BACKENDS_CUDNN_POOLING_ENGINE_H
