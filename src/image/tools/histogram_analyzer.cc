#include "histogram_analyzer.h"
#include <numeric>
#include <cmath>

namespace image {

Histogram HistogramAnalyzer::Compute(const ImageBuffer& img) const {
    Histogram h;
    if (!img.IsValid()) return h;
    const uint8_t* d = img.Data();
    int n = img.Width() * img.Height();
    for (int i = 0; i < n; ++i) {
        h.r[d[i*4+0]]++;
        h.g[d[i*4+1]]++;
        h.b[d[i*4+2]]++;
        h.a[d[i*4+3]]++;
        uint8_t lum = static_cast<uint8_t>(
            0.2126f*d[i*4+0] + 0.7152f*d[i*4+1] + 0.0722f*d[i*4+2]);
        h.luminance[lum]++;
    }
    return h;
}

float HistogramAnalyzer::MeanLuminance(const Histogram& h) const {
    uint64_t sum = 0, total = 0;
    for (int i = 0; i < 256; ++i) { sum += i * h.luminance[i]; total += h.luminance[i]; }
    return total ? static_cast<float>(sum) / total : 0.0f;
}

ImageBuffer HistogramAnalyzer::Equalize(const ImageBuffer& img) const {
    ImageBuffer out = img.Clone();
    if (!out.IsValid()) return out;
    auto h = Compute(img);
    int n = img.Width() * img.Height();
    // Build LUT per channel from CDF
    auto make_lut = [&](const std::array<uint32_t,256>& hist) {
        std::array<uint8_t,256> lut{};
        uint64_t cdf = 0, cdf_min = 0;
        for (int i=0;i<256;++i) if (hist[i]) { cdf_min=hist[i]; break; }
        for (int i=0;i<256;++i) {
            cdf += hist[i];
            lut[i] = static_cast<uint8_t>(
                std::round((float)(cdf - cdf_min) / (n - cdf_min) * 255));
        }
        return lut;
    };
    auto lr = make_lut(h.r);
    auto lg = make_lut(h.g);
    auto lb = make_lut(h.b);
    uint8_t* d = out.Data();
    for (int i=0;i<n;++i) {
        d[i*4+0]=lr[d[i*4+0]];
        d[i*4+1]=lg[d[i*4+1]];
        d[i*4+2]=lb[d[i*4+2]];
    }
    return out;
}

}  // namespace image
