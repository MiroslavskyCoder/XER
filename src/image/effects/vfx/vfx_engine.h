#pragma once
#include "../../core/image_buffer.h"
#include "../filters/filter_chain.h"

namespace image {

/// High-level VFX pipeline built on FilterChain.
class VfxEngine {
public:
    VfxEngine() = default;

    FilterChain& Filters() { return chain_; }
    const FilterChain& Filters() const { return chain_; }

    void AddBlur(double sigma, bool gaussian = true);
    void AddSharpen(double amount = 1.0, double sigma = 1.0);
    void AddGrayscale();
    void AddSepia(float amount = 1.0f);
    void AddInvert();
    void AddEdgeDetect();
    void AddEmboss(double angle = 315.0, double strength = 1.5);
    void AddVignette(double strength = 0.5, double feather = 0.7);

    void Process(ImageBuffer& img) const;
    void Reset() { chain_.Clear(); }

private:
    FilterChain chain_;
};

}  // namespace image
