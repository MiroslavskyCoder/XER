#pragma once
#include "../../core/image_buffer.h"

namespace image {

class SharpenFilter {
public:
    explicit SharpenFilter(double amount=1.0, double sigma=1.0, int threshold=0)
        : amount_(amount), sigma_(sigma), threshold_(threshold) {}
    void Apply(ImageBuffer& img) const;
private:
    double amount_;
    double sigma_;
    int    threshold_;
};

}  // namespace image
