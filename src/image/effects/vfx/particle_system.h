#pragma once
#include "../../core/image_buffer.h"
#include <vector>
#include <random>
#include <cstdint>

namespace image {

struct Particle {
    float    x, y;
    float    vx, vy;
    float    life;    ///< 1.0 = full, 0.0 = dead
    float    size;
    uint32_t color;   ///< Skia-packed RGBA
};

class ParticleSystem {
public:
    explicit ParticleSystem(int capacity = 500);

    void Emit(float x, float y, int count = 10);
    void Update(float dt_seconds);
    void Render(ImageBuffer& img) const;
    void SetGravity(float gx, float gy) { gx_ = gx; gy_ = gy; }
    std::size_t Count() const { return particles_.size(); }

private:
    std::vector<Particle> particles_;
    int      capacity_;
    float    gx_ = 0.0f, gy_ = 98.0f;
    mutable std::mt19937 rng_;
};

}  // namespace image
