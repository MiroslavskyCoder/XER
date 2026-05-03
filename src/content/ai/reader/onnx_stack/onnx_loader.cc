#include "onnx_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

// In production, would include <onnx/onnx_pb.h> from ONNX proto
// For now, stub implementation

namespace Engine::ModelsBuilder::Reader::Onnx {

std::shared_ptr<Core::Model> OnnxLoader::Load(const std::string& filepath) {
    // Check file exists
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open ONNX file: " + filepath);
    }
    
    // Read file into buffer
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();
    
    return LoadFromBuffer(buffer.data(), buffer.size());
}

std::shared_ptr<Core::Model> OnnxLoader::LoadFromBuffer(const uint8_t* buffer,
                                                         size_t size) {
    if (!buffer || size == 0) {
        throw std::invalid_argument("Invalid buffer or size");
    }
    
    // TODO: Parse ONNX protobuf format
    // Steps:
    // 1. Parse ONNX ModelProto
    // 2. Validate graph structure
    // 3. Infer shapes
    // 4. Convert each ONNX node to XER layer
    // 5. Build output model
    
    // Stub: create empty model
    auto model = std::make_shared<Core::Model>("OnnxModel");
    
    // In production:
    // - Use onnx::ModelProto to deserialize
    // - Extract graph, initializers, inputs, outputs
    // - For each node, create corresponding XER layer via LayerFactory
    
    return model;
}

std::string OnnxLoader::GetMetadata(const std::string& filepath) {
    // TODO: Extract ONNX metadata (ir_version, producer_name, opset_version, etc.)
    std::ostringstream ss;
    ss << "ONNX model: " << filepath;
    return ss.str();
}

} // namespace Engine::ModelsBuilder::Reader::Onnx
