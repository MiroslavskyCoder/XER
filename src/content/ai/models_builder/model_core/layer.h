#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <string_view>

namespace Engine::ModelsBuilder::Core {

enum class LayerType : uint8_t {
    Dense = 0,
    Conv2D = 1,
    MaxPool = 2,
    Activation = 3,
    Dropout = 4,
    BatchNorm = 5,
    Flatten = 6,
    LSTM = 7
};

enum class ActivationType : uint8_t {
    ReLU = 0,
    Sigmoid = 1,
    Tanh = 2,
    Linear = 3,
    SoftMax = 4
};

class Layer {
public:
    explicit Layer(LayerType type);
    virtual ~Layer() = default;
    
    LayerType GetLayerType() const { return layer_type_; }
    const std::string& GetLayerName() const { return layer_name_; }
    const std::vector<uint32_t>& GetOutputShape() const { return output_shape_; }
    
    void SetLayerName(const std::string& name) { layer_name_ = name; }
    
    virtual bool ValidateInputShape(const std::vector<uint32_t>& input_shape) const;
    virtual std::vector<uint32_t> ComputeOutputShape(const std::vector<uint32_t>& input_shape);
    virtual bool Initialize() { return true; }
    virtual std::string Describe() const;

protected:
    LayerType layer_type_;
    std::string layer_name_;
    std::vector<uint32_t> output_shape_;
};

} // namespace Engine::ModelsBuilder::Core
