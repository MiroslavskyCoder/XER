#pragma once

#include "layer.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Engine::ModelsBuilder::Core {

enum class ModelType : uint8_t {
    Sequential = 0,
    Functional = 1,
    Subclass = 2
};

class Model {
public:
    explicit Model(const std::string& name = "Model");
    virtual ~Model() = default;
    
    const std::string& GetModelName() const { return model_name_; }
    ModelType GetModelType() const { return model_type_; }
    bool IsCompiled() const { return is_compiled_; }
    const std::vector<uint32_t>& GetInputShape() const { return input_shape_; }
    const std::vector<uint32_t>& GetOutputShape() const { return output_shape_; }
    const std::vector<std::shared_ptr<Layer>>& GetLayers() const { return layers_; }
    
    void SetModelName(const std::string& name) { model_name_ = name; }
    void SetModelType(ModelType type) { model_type_ = type; }
    
    virtual void AddLayer(std::shared_ptr<Layer> layer);
    virtual void ClearLayers();
    virtual bool Build(const std::vector<uint32_t>& input_shape);
    virtual bool Compile();
    
    size_t GetLayerCount() const { return layers_.size(); }
    std::shared_ptr<Layer> GetLayer(size_t index) const;

protected:
    std::string model_name_;
    ModelType model_type_;
    std::vector<std::shared_ptr<Layer>> layers_;
    bool is_compiled_;
    std::vector<uint32_t> input_shape_;
    std::vector<uint32_t> output_shape_;
};

} // namespace Engine::ModelsBuilder::Core
