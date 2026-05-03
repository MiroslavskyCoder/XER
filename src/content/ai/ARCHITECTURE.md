# AI Module Architecture

**Comprehensive technical architecture documentation for XER's AI/ML framework**

## 1. System Architecture

### High-Level Stack

```
┌─────────────────────────────────────────────────────┐
│  Application Layer (JavaScript Engine Bindings)     │
├─────────────────────────────────────────────────────┤
│  Public API (Model Building, Training, Inference)   │
├─────────────────────────────────────────────────────┤
│  Model Builder Layer (Sequential/Functional Models) │
├─────────────────────────────────────────────────────┤
│  Optimizer & Loss Functions (SGD, Adam, MSE, CE)    │
├─────────────────────────────────────────────────────┤
│  ML Data Pipeline (Load → Preprocess → Cache)       │
├─────────────────────────────────────────────────────┤
│  High-Level Operators (MatMul, Conv, ReLU, etc.)    │
├─────────────────────────────────────────────────────┤
│  XNNPACK (Inference) | Eigen (Compute)              │
├─────────────────────────────────────────────────────┤
│  CUDA/CuDNN | CPU BLAS | Quantization (fp16/int8)   │
├─────────────────────────────────────────────────────┤
│  NVIDIA CUTLASS | GPU Kernels | pthreadpool Workers │
└─────────────────────────────────────────────────────┘
```

## 2. Core Components

### 2.1 Model Architecture

#### Layer Hierarchy
```cpp
Layer (abstract base)
├── DenseLayer           [Fully connected, weights=in×out]
├── ConvLayer            [2D convolution, kernels+biases]
├── ActivationLayer      [ReLU, Sigmoid, Tanh, etc.]
├── BatchNormLayer       [Batch normalization]
├── PoolingLayer         [Max/Avg pooling]
├── DropoutLayer         [Stochastic regularization]
└── CustomLayer          [User-defined operations]
```

#### Model Types
```cpp
enum class ModelType : uint8_t {
    Sequential = 0,    // Layer stacking: Input → L1 → L2 → Output
    Functional = 1,    // DAG model: multiple inputs/outputs
    Subclass = 2       // Custom class definition (Python-like)
};
```

#### Compilation Process
```
Model.Build(input_shape)
    └─ For each layer:
        ├─ ValidateInputShape(current_shape)
        ├─ Initialize()
        └─ ComputeOutputShape() → next layer input
    └─ Set input_shape_, output_shape_

Model.Compile()
    └─ Validate layers exist and built
    └─ Freeze topology (no AddLayer allowed after)
    └─ Lock configuration for training
```

### 2.2 Training Pipeline

#### Trainer Architecture
```
Trainer(Model&)
├── Forward Pass (inference)
│   └─ For each layer: layer.Forward(input) → output
│
├── Loss Computation
│   └─ LossFunction::Compute(predictions, targets)
│
├── Backward Pass (gradient computation)
│   └─ For each layer (reverse): layer.Backward(gradient)
│
└── Optimizer Update
    └─ Optimizer.UpdateWeights(learning_rate, gradients)
```

#### Supported Optimizers
- **SGD** (Stochastic Gradient Descent)
- **Momentum SGD**
- **Adam** (Adaptive Moment Estimation)
- **RMSprop**

#### Loss Functions
- **Mean Squared Error (MSE)** - Regression
- **Cross-Entropy** - Classification
- **Binary Cross-Entropy** - Binary classification
- **Custom Loss** - User-defined

### 2.3 Inference Engine (XNNPACK)

#### Optimization Strategy
```
Model → Graph Fusion → Operator Fusion → Execution
    └─ Conv+ReLU → fused kernel
    └─ MatMul+Add+ReLU → single operation
    └─ Redundant ops elimination
    └─ XNNPACK lightweight inference
```

#### Performance Benefits
- **Operator Fusion**: 1 kernel invocation instead of 3
- **Quantization**: fp32 → int8 (4x memory reduction)
- **Precision**: fp32 training, fp16/int8 inference
- **Latency**: 10-50ms for typical models

## 3. Data Pipeline

### 3.1 Data Loading Architecture

```
Source (CSV/Image files)
    ↓
DataLoader (pthreadpool workers)
├─ Reader threads (load raw bytes)
├─ Preprocessor threads (normalize/scale)
└─ Batcher threads (group into batches)
    ↓
Cache (in-memory deduplicated)
    ↓
Training Loop (prefetch next batch)
```

#### Supported Formats
- **CSV**: Tabular data with fast parsing
- **Images**: PNG, JPEG, BMP via OpenCV
- **Binary**: Raw tensor dumps
- **Datasets**: PyTorch-compatible formats

### 3.2 Data Preprocessing

#### Normalization Strategies
1. **StandardScaler**: (X - mean) / std
2. **MinMaxScaler**: (X - min) / (max - min)
3. **RobustScaler**: (X - median) / IQR
4. **Whitening**: Eigendecomposition-based correlation removal

#### Augmentation (OpenCV-based)
```cpp
RotationAugmentor     [0°-360°]
FlipAugmentor         [Horizontal, Vertical]
ZoomAugmentor         [0.8-1.2x scale]
TranslationAugmentor  [Spatial shift]
NoiseAugmentor        [Gaussian noise injection]
ColorJitterAugmentor  [Brightness/Contrast/Hue]
```

## 4. Library Integration

### 4.1 CUDA/CuDNN Integration

#### CUDA Operations
```cpp
class CudaTensorKernel {
    // Async copy: Host ↔ Device
    CudaAsyncCopy(src, dst, size, stream)
    
    // Elementwise operations: C = op(A, B)
    CudaElementwise(operation, A, B, C, size, stream)
    
    // Reduction: scalar = reduce(A, op)
    CudaReduceOps(operation, A, result, size, stream)
    
    // Stream management for pipelining
    CudaStreamOps(create, synchronize, destroy)
};
```

#### CuDNN Bindings
- **Convolution**: Conv2D forward/backward, auto-tuning
- **Pooling**: MaxPool, AvgPool
- **Activation**: ReLU, Sigmoid, Tanh
- **BatchNorm**: Forward/backward with running stats
- **RNN**: LSTM, GRU, Vanilla RNN

### 4.2 Eigen Integration

#### Tensor Abstractions
```cpp
// Eigen matrix operations
Eigen::MatrixXf weights(128, 64);  // Dynamic size
Eigen::MatrixXf input(batch, 64);
auto output = input * weights;      // GEMM via CUTLASS

// Broadcasting
Eigen::MatrixXf bias = bias_vector.replicate(batch, 1);

// Slicing
auto row = weights.row(0);
auto block = weights.block(0, 0, 64, 32);
```

#### CUTLASS Integration
```cpp
// High-performance matrix multiplication
using Gemm = cutlass::gemm::device::Gemm<
    cutlass::half_t,              // Element type (fp16)
    cutlass::layout::RowMajor,    // Layout A
    cutlass::layout::ColumnMajor, // Layout B
    cutlass::float32,             // Accumulator type
    cutlass::OpMultiplyAddSaturate>;

Gemm gemm_op;
gemm_op({M, N, K}, input_ptr, output_ptr, alpha, beta);
```

### 4.3 XNNPACK Integration

#### Inference API
```cpp
class ModelPredictor {
    // Convert model to XNNPACK subgraph
    XNNPACK::Subgraph graph;
    
    // Define operators with fusions
    AddOperator(operation, inputs, outputs);
    
    // Create runtime
    runtime = xnn_create_runtime(graph);
    
    // Predict (batch processing)
    Predict(tensor) → fused kernels → output
};
```

#### Operator Fusion Examples
```
Conv2D(3,3,64) + ReLU → single kernel
    └─ Memory: 1 read/write cycle
    └─ Latency: 3x faster than separate ops

MatMul + Add + ReLU → single kernel
    └─ GEMMDirect optimized for this pattern

Conv2D → BatchNorm → ReLU → Conv2D (2x)
    └─ Full chain fusion for ResNet-like blocks
```

### 4.4 FlatBuffers Serialization

#### Schema Design
```flatbuffers
table Model {
    name: string (required);
    version: uint32;
    input_shape: [uint32] (required);
    output_shape: [uint32] (required);
    layers: [Layer] (required);
}

table Layer {
    type: uint8 (required);      // 0=Dense, 1=Conv, etc.
    name: string;
    parameters: [Parameter];
}

table Parameter {
    name: string (required);
    dtype: uint8;                // 0=fp32, 1=fp16, 2=int8
    shape: [uint32];
    data: [ubyte];               // Compressed tensor data
}
```

#### Serialization Workflow
```
Model instance
    ↓
Serialize weights (compression, quantization)
    ↓
Build FlatBuffers table
    ↓
Write to binary file
    ↓
[Deserialization]
    ↓
Read FlatBuffers
    ↓
Decompress weights
    ↓
Restore Model instance
```

### 4.5 pthreadpool Integration

#### Work-Stealing Thread Pool
```cpp
pthreadpool_t threadpool = pthreadpool_create(num_threads);

// Parallel for loop
pthreadpool_parallelize_1d(
    threadpool,
    task_function,  // void(void*, size_t index)
    context,        // User data
    num_iterations
);

// Parallel 2D tile
pthreadpool_parallelize_2d_tile_1d(
    threadpool,
    tile_function,
    context,
    H, W, tile_h, tile_w
);

pthreadpool_destroy(threadpool);
```

#### Data Loading with pthreadpool
```
Main Thread
    ├─ Worker 1: Read batch 0 from disk
    ├─ Worker 2: Preprocess batch 1 (normalize, augment)
    ├─ Worker 3: Add batch 2 to cache
    ├─ Main: Start training on batch 3 (GPU)
    └─ Workers: Prefetch next batches while training
```

### 4.6 ONNX Integration

#### Model Import Pipeline
```
ONNX File
    ↓
Parse (.onnx binary)
    ↓
Validate Graph (shapes, types, operators)
    ↓
Optimize Graph
    ├─ Operator Fusion (Conv+ReLU, Add+ReLU)
    ├─ Constant Folding
    ├─ Dead Code Elimination
    └─ Shape Inference
    ↓
Convert to XER Model
    ├─ ONNX Ops → XER Layers
    ├─ Quantization (optional)
    └─ GPU binding
    ↓
Ready for Inference/Training
```

#### Supported ONNX Operators
- **Activation**: ReLU, Sigmoid, Tanh, Softmax, LeakyReLU
- **Convolution**: Conv2D, ConvTranspose2D
- **Pooling**: MaxPool, AveragePool
- **Normalization**: BatchNormalization, LayerNormalization
- **Linear**: Gemm, MatMul, FullyConnected
- **Reduction**: Reduce* (Sum, Mean, Max, Min)
- **Advanced**: LSTM, GRU, Attention

## 5. Concurrency & Threading Model

### 5.1 Thread Roles

```
Main Thread (Engine)
├─ Model instantiation
├─ Model compilation
└─ Training orchestration

GPU Stream (CUDA)
├─ Forward passes (compute kernels)
├─ Backward passes (gradient computation)
└─ Weight updates (sparse operations)

Data Loaders (pthreadpool workers)
├─ File I/O (CSV, images)
├─ Preprocessing (normalize, augment)
├─ Caching (deduplicate samples)
└─ Batching (group samples)
```

### 5.2 Synchronization Points

```
Epoch Start
    ↓
Main Thread: EnqueueNextBatch() → pthreadpool
    ↓
Workers: Load/preprocess in parallel
    ↓
Main Thread: WaitForBatch() → blocks if not ready
    ↓
Main Thread: Forward/Backward/Update on GPU
    ↓
[Repeat until epoch complete]
    ↓
pthreadpool::Destroy()
    ↓
GPU::Synchronize() [ensure all kernels done]
    ↓
Epoch Complete
```

## 6. Memory Management

### 6.1 Allocation Strategy

```
Persistent (Model Lifetime)
├─ Weights: Device + pinned host copy (for I/O)
├─ Biases: Device
└─ Batch Norm params: Device

Temporary (Forward Pass)
├─ Activations: Device (cached for backward)
├─ Intermediate: Device
└─ Gradients: Device (during backward pass)

Cache (Data Loading)
├─ Deduplication: Host RAM
├─ Prefetch: Host → Device pipeline
└─ LRU eviction when full
```

### 6.2 RAII for GPU Memory

```cpp
class CudaBuffer {
    CudaBuffer(size_t bytes) {
        cudaMalloc(&data_, bytes);
    }
    
    ~CudaBuffer() {
        cudaFree(data_);  // Automatic cleanup
    }
    
    void* Get() { return data_; }
    
private:
    void* data_;
};

// Usage
{
    CudaBuffer buffer(1024 * 1024);  // 1MB
    Compute(buffer.Get());
}  // Auto freed when scope exits
```

## 7. Error Handling & Logging

### 7.1 Error Hierarchy

```
ModelBuilderErrorCode (enum)
├── InvalidArgument      [NULL pointer, bad type]
├── InvalidShape         [Shape mismatch, empty shape]
├── InitializationFailed [Layer setup error]
├── CompilationFailed    [Model topology invalid]
├── ModelHasNoLayers     [Empty model]
├── CudaError            [GPU operation failure]
├── FileIOError          [Load/save failure]
└── Custom              [User-defined errors]
```

### 7.2 Logging Infrastructure

```cpp
ModelBuilderLogger logger;
logger.SetMinimumLevel(ModelBuilderLogLevel::Debug);
logger.EnableConsoleOutput(true);

logger.Debug("Starting training...");
logger.Info("Epoch 1: loss=0.234");
logger.Warning("Learning rate very high: 1.0");
logger.Error("GPU out of memory!");

auto entries = logger.GetEntries();  // Retrieve all logs
```

## 8. Performance Optimization Techniques

### 8.1 Computation Graph Optimization

```
Original Graph:
    input → Conv2D → ReLU → BatchNorm → ReLU → Conv2D → output
    
Optimized Graph (fused):
    input → [Conv2D+ReLU] → [BatchNorm] → [ReLU+Conv2D] → output
    
Benefit: 3 kernel launches → 2 kernel launches
```

### 8.2 Weight Quantization

```
fp32 weights (8GB)
    ↓
Calibrate min/max ranges
    ↓
Quantize to int8 (-128 to 127)
    ↓
int8 weights (1GB) + scale/zero_point
    ↓
Dequantize on GPU before compute
    ↓
Result: 4x memory reduction, <1% accuracy loss
```

### 8.3 Mixed Precision Training

```
Forward Pass:
    input (fp32) → ops (fp16) → output (fp32)
    
Backward Pass:
    gradient (fp32) → ops (fp16) → weight_grad (fp32)
    
Weight Update:
    weights (fp32) += learning_rate * weight_grad
    
Benefit: 2x faster computation, minimal accuracy impact
```

## 9. Testing & Validation

### 9.1 Unit Tests

```cpp
TEST(ModelBuilderTest, BuildWithValidShape) {
    Model model("test");
    model.AddLayer(std::make_shared<DenseLayer>(64, 128));
    EXPECT_TRUE(model.Build({1, 64}));
    EXPECT_EQ(model.GetOutputShape()[1], 128);
}

TEST(TrainerTest, SingleEpoch) {
    // Create model, dataset, trainer
    // Run 1 epoch
    // Verify loss decreased
}
```

### 9.2 Smoke Tests

- Model serialization/deserialization round-trip
- ONNX model import accuracy validation
- End-to-end training on small dataset
- Inference performance on large batch

## 10. Future Enhancements

### Planned Features

1. **Distributed Training**
   - Multi-GPU AllReduce
   - Gradient compression
   - Async SGD

2. **Advanced Architectures**
   - Vision Transformers (ViT)
   - Diffusion Models (full pipeline)
   - Large Language Models (LLMs)

3. **Quantization-Aware Training**
   - Simulated quantization during training
   - Learnable clipping ranges
   - Per-channel vs per-layer quantization

4. **AutoML**
   - Neural Architecture Search (NAS)
   - Hyperparameter tuning
   - Model compression (pruning, distillation)

5. **Advanced I/O**
   - Distributed data loading (NFS, S3)
   - Streaming datasets
   - Synthetic data generation

---

**Architecture Version**: 1.0  
**Last Updated**: May 2026  
**Maintainers**: XER Runtime Team
