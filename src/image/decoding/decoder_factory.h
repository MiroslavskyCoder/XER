#pragma once
#include "base_decoder.h"
#include <memory>
#include <vector>
#include <string>

namespace image {

/// Singleton factory: selects decoder by magic bytes or extension.
class DecoderFactory {
public:
    static DecoderFactory& Instance();

    /// Register a decoder (called once per format at startup).
    void Register(std::unique_ptr<BaseDecoder> decoder);

    /// Find decoder for file (probes magic bytes, falls back to extension).
    BaseDecoder* Find(const std::string& path);

    /// Find decoder from raw bytes (magic detection only).
    BaseDecoder* FindByMagic(const uint8_t* header, std::size_t len);

    /// Decode helper: find + decode from path.
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc);

    void RegisterBuiltins();  ///< Register all standard format decoders.

private:
    DecoderFactory() = default;
    std::vector<std::unique_ptr<BaseDecoder>> decoders_;
    bool builtins_registered_ = false;
};

}  // namespace image
