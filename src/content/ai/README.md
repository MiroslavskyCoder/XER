# XER AI Module

**Модуль искусственного интеллекта** (AI Module) — comprehensive neural network and machine learning framework for the XER runtime.

## Overview | Обзор

The AI module provides a complete, GPU-accelerated machine learning infrastructure for:
- **Neural Network Construction** - Sequential and functional model building
- **Training & Optimization** - Multi-threaded training with CUDA acceleration
- **Inference** - High-performance model inference with XNNPACK optimization
- **Data Processing** - Complete ML data pipeline (loading, preprocessing, augmentation)
- **Model Serialization** - FlatBuffers-based model persistence
- **Framework Integration** - ONNX, DarkNet, OpenVINO model import

### Dependencies | Зависимости

| Library | Purpose | Version |
|---------|---------|---------|
| **CUDA** | GPU computation backend | 11.x+ |
| **CuDNN** | Optimized deep learning operations | 8.x+ |
| **NVIDIA CUTLASS** | Optimized matrix multiplication kernels | 3.x+ |
| **Eigen3** | Linear algebra and tensor operations | 3.4+ |
| **OpenCV** | Computer vision and image augmentation | 4.5+ |
| **XNNPACK** | Lightweight inference acceleration | Latest |
| **FlatBuffers** | Efficient serialization format | 2.0+ |
| **ONNX Runtime** | ONNX model execution | 1.14+ |
| **ONNX Optimizer** | Graph optimization | Latest |
| **pthreadpool** | Efficient work-stealing thread pool | Latest |
| **fp16** | Half-precision floating point utilities | Latest |

## Architecture | Архитектура

```
XER AI Module
│
├── models_builder/          [Модель construction & training]
│   ├── model_core/          - Core layer and model classes
│   ├── model_builder/       - Model factory and builders
│   ├── model_inference/     - Inference engine (XNNPACK)
│   ├── model_optimization/  - Optimizer and loss functions
│   ├── model_training/      - Training orchestration
│   ├── model_serialization/ - FlatBuffers serialization
│   ├── weight_management/   - Weight initialization & validation
│   ├── graph_engine/        - Computation graph
│   ├── hardware_binding/    - GPU/CPU binding strategies
│   ├── training_metrics/    - Metrics collection
│   └── utility/             - Runtime feature detection + thread pool bridge
│
├── ml/                      [Machine Learning utilities]
│   ├── cuda_ops/            - CUDA kernel operations
│   ├── tensors/             - Eigen tensor abstractions
│   ├── data_types/          - Batch, Dataset, Tensor definitions
│   ├── data_loading/        - CSV, Image, generic loaders
│   ├── data_preprocessing/  - Normalization, scaling, preprocessing
│   ├── data_augmentation/   - OpenCV-based augmentation
│   ├── data_caching/        - In-memory dataset caching
│   ├── data_validation/     - Data integrity validation
│   ├── data_structures/     - Advanced data structures
│   ├── features/            - Feature extraction pipelines
│   ├── augmentation/        - Advanced augmentation techniques
│   └── utils/               - ML utilities
│
├── reader/                  [Model format adapters]
│   ├── onnx_stack/          - ONNX model loading & optimization
│   ├── darknet_stack/       - YOLO/DarkNet model import
│   ├── vino_stack/          - OpenVINO model support
│   ├── framework_adapters/  - Framework-specific adapters
│   ├── schema_validation/   - Model format validation
│   ├── transformation/      - Model graph transformation
│   ├── core_loaders/        - Base loader implementations
│   ├── error_handling/      - Reader error management
│   └── utils/               - Reader utilities
│
├── img2img/                 [Image-to-image generation]
│   └── (Stable Diffusion img2img pipeline)
│
├── txt2img/                 [Text-to-image generation]
│   └── (Stable Diffusion txt2img pipeline)
│
├── sd_base/                 [Stable Diffusion foundation]
│   ├── core/                - Core diffusion process
│   ├── layer/               - Custom layer implementations
│   ├── gpu/                 - GPU-specific optimizations
│   ├── tensor/              - Tensor utilities
│   ├── controlnet/          - ControlNet integration
│   ├── depth/               - Depth control
│   └── (... other specialized components)
│
└── agent/                   [Autonomous AI agents]
    └── (Planning stage)
```

## Module Responsibilities

### models_builder | Построение моделей
Complete neural network construction and training framework:
- **model_core**: Layer and Model abstractions
- **model_builder**: Factory for creating different model architectures
- **model_training**: Training loops with gradient computation
- **model_inference**: XNNPACK-accelerated inference
- **weight_management**: Xavier, He, and other initialization strategies
- **model_serialization**: FlatBuffers format for efficient storage

### ml | Machine Learning
Core ML infrastructure and data pipeline:
- **cuda_ops**: Low-level CUDA operations (async copy, elementwise, reduce)
- **tensors**: Eigen-based tensor abstractions with CPU/GPU dispatch
- **data_types**: Batch, Dataset, Tensor primitives
- **data_loading**: Multi-threaded data loaders for CSV and images
- **data_preprocessing**: Normalization, standardization, scaling
- **data_augmentation**: Random crops, rotations, flips using OpenCV

### reader | Model Import
Framework-independent model loading:
- **onnx_stack**: Full ONNX support with graph optimization
- **darknet_stack**: YOLO/DarkNet weight conversion
- **vino_stack**: OpenVINO IR import
- **schema_validation**: Verify model integrity
- **transformation**: Graph fusion and optimization

### sd_base, img2img, txt2img | Generative AI
Stable Diffusion implementation (planning/skeleton):
- Core diffusion process with noise scheduling
- UNet backbone architecture
- VAE encoder/decoder
- Text encoder integration
- ControlNet for fine-grained control
- GPU optimizations with CUDA/TensorRT

## Usage | Использование

### Building a Model
```cpp
using namespace Engine::ModelsBuilder;

// Create model
auto model = std::make_unique<Core::Model>("MyModel");

// Add layers
auto dense1 = std::make_unique<Core::DenseLayer>(128, 64);
model->AddLayer(std::move(dense1));

// Build and compile
model->Build({1, 64});
model->Compile();
```

### Training
```cpp
Core::Trainer trainer(*model);
trainer.SetLearningRate(0.001f);
trainer.SetBatchSize(32);
trainer.Train(dataset, 10);  // 10 epochs
```

### Inference
```cpp
auto predictor = std::make_unique<Inference::ModelPredictor>(*model);
auto output = predictor->Predict(input_tensor);
```

### Loading ONNX Model
```cpp
using namespace Engine::ModelsBuilder::Reader;

auto onnx_loader = std::make_unique<OnnxLoader>();
auto model = onnx_loader->Load("model.onnx");
```

## Design Patterns | Паттерны проектирования

1. **Singleton Pattern** - Logger, ErrorHandler, Config
2. **Factory Pattern** - LayerFactory, ModelBuilder, LoaderFactory
3. **Observer Pattern** - Training metrics collection
4. **Strategy Pattern** - Weight initialization, loss functions, optimizers
5. **RAII** - Automatic resource management for tensors, CUDA memory
6. **Async I/O** - Non-blocking data loading with pthreadpool

## Performance Characteristics

| Operation | Optimization |
|-----------|--------------|
| Matrix Multiplication | CUTLASS kernels, GEMM fusion |
| Tensor Operations | Eigen vectorization, CUDA kernels |
| Data Loading | pthreadpool work-stealing + prefetching |
| Model Inference | XNNPACK operator fusion, quantization |
| Model Training | Mixed precision (fp16 + fp32) with fp16 library |
| Memory | Smart caching, gradient accumulation support |

## Thread Safety

- **Logger, ErrorHandler** - Thread-safe singleton with mutexes
- **Data Loaders** - pthreadpool-based thread-safe prefetching
- **Model Inference** - Single-threaded (thread pool managed at Engine level)
- **Training** - Gradient computation parallelization via OpenMP

## Runtime Integration (New)

- `models_builder/utility/ai_runtime_features.*` performs compile-time capability checks for: range-v3, absl, zlib, icu, libuv, CUDA, CuDNN, CUTLASS, Eigen, OpenCV, XNNPACK, FlatBuffers, OpenVINO, ONNX, TensorFlow, pthreadpool, fp16.
- `models_builder/utility/mb_thread_pool.*` is now implemented and bridged to `src/async_io/io_thread_pool.*` to reuse Engine-wide worker infrastructure.
- `models_builder/utility/mb_memory_profiler.*` now provides runtime memory snapshots and peak resident/virtual usage tracking.
- `model_core/model.cc` now emits a runtime banner at compile stage showing enabled AI feature groups.
- `reader/onnx_stack/onnx_graph_parser.*` now builds a lightweight ONNX graph representation from binary buffers and extracts metadata.
- `reader/onnx_stack/onnx_loader.*` now applies optimizer passes, converts graph nodes to XER layers, builds/compiles the resulting model, and supports byte-level caching.
- `reader/utils/rm_cache_manager.*` and `reader/utils/rm_logger.*` now provide reusable model-byte cache and structured reader logging.
- `reader/framework_adapters/onnx_model_reader.*` and `reader/framework_adapters/openvino_ir_reader.*` are integrated into `ReaderPipeline` through `ReaderFactory`.
- `reader/core_loaders/reader_pipeline.*` now exposes unified quality-check flow (`ProcessWithQuality`) with schema validation, metadata extraction, and integrity verification.
- `reader/onnx_stack/onnx_node_map.*`, `onnx_attribute_reader.*`, and `onnx_tensor_converter.*` are now implemented and used by ONNX conversion.
- `models_builder/model_inference/predictor.cc` now uses optional Eigen, CUTLASS, CUDA/CuDNN hooks and XNNPACK backend delegation with safe fallback.
- `models_builder/model_training/trainer.cc` now supports backend-aware acceleration (parallel batches, Eigen/OpenCV paths, runtime backend capability logging).
- `models_builder/training_metrics/*` now provides working metric collectors for accuracy, loss tracking, gradient norms, bucketed timing (with RAII scoped timer), and resource usage snapshots.
- `models_builder/hardware_binding/*` now provides CUDA kernel planning, CuDNN/OpenVINO runtime configuration adapters, ONNX-like model export, and TensorRT conversion descriptor generation with runtime capability fallback.
- `models_builder/checkpointing/*` now provides metadata serialization, binary checkpoint snapshots, checkpoint manager (save/load/list/prune), and rollback restore flow on top of model serialization.
- `models_builder/model_operations/*` now provides clone/compress/export/fuse/merge/prune/quantize/split/update/version operations for model lifecycle management.
- `ml/utils/*` now provides working utilities: logger, math ops (with optional Eigen path), profiler, random generator, config parser, type converter (with fp16 round-trip), and backend-aware version manager.
- `ml/data_structures/*` now provides working containers and pipelines: thread-safe queues, buffer queue, memory pool, shared memory segment abstraction, dataset base/iterator/shuffle/sampler/window, batch builder, dataset map, and feature cache manager.

## Error Handling

Comprehensive error reporting:
```cpp
// Errors are captured and logged
try_operation();

// Access errors via singleton
auto& error_handler = Utility::ModelBuilderErrorHandler::GetInstance();
if (error_handler.HasErrors()) {
    auto last_error = error_handler.GetLastError();
    // Handle error
}
```

## Testing

Smoke tests in `demo_app/`:
- `ml_module_smoke.js` - ML pipeline testing
- `models_smoke.js` - Model construction/training
- Training/inference correctness validation

## Contributing | Вклад

When enhancing the AI module:
1. Maintain Chromium code style (2-space indents, C++17)
2. Add comprehensive inline documentation
3. Use library abstractions (Eigen, CUDA, XNNPACK)
4. Include thread-safety considerations
5. Update this README with new functionality

## References | Ссылки

- **NVIDIA CUDA**: https://docs.nvidia.com/cuda/
- **NVIDIA cuDNN**: https://docs.nvidia.com/cudnn/
- **NVIDIA CUTLASS**: https://github.com/NVIDIA/cutlass
- **Eigen**: https://eigen.tuxfamily.org/
- **ONNX**: https://onnx.ai/
- **XNNPACK**: https://github.com/google/XNNPACK
- **FlatBuffers**: https://flatbuffers.dev/
- **fp16**: https://github.com/Maratyszcza/FP16
- **pthreadpool**: https://github.com/Maratyszcza/pthreadpool

---

**Maintainers**: XER Runtime Team  
**Last Updated**: May 2026  
**Status**: Active Development
