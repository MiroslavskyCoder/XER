#include "crop_tool.h"
#include <cstring>
#include <algorithm>

namespace image {

ImageBuffer CropTool::Crop(const ImageBuffer& img, int x, int y, int w, int h) {
    if (!img.IsValid()) return {};
    x = std::clamp(x, 0, img.Width()-1);
    y = std::clamp(y, 0, img.Height()-1);
    w = std::min(w, img.Width()-x);
    h = std::min(h, img.Height()-y);
    if (w <= 0 || h <= 0) return {};
    ImageBuffer out(w, h, img.Format(), img.GetColorSpace());
    const uint8_t* src = img.Data();
    uint8_t* dst = out.Data();
    int bpp = BytesPerPixel(img.Format());
    for (int row=0;row<h;++row)
        std::memcpy(dst + row*w*bpp, src + ((y+row)*img.Width()+x)*bpp, w*bpp);
    return out;
}

ImageBuffer CropTool::Pad(const ImageBuffer& img, int nw, int nh,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!img.IsValid()) return {};
    ImageBuffer out(nw, nh, img.Format(), img.GetColorSpace());
    out.Clear(r, g, b, a);
    int ox = (nw - img.Width()) / 2, oy = (nh - img.Height()) / 2;
    int bpp = BytesPerPixel(img.Format());
    const uint8_t* src = img.Data();
    uint8_t* dst = out.Data();
    for (int row=0;row<img.Height();++row)
        std::memcpy(dst + ((oy+row)*nw+ox)*bpp, src + row*img.Width()*bpp, img.Width()*bpp);
    return out;
}

ImageBuffer CropTool::Rotate90(const ImageBuffer& img, int times) {
    ImageBuffer cur = img.Clone();
    for (int t=0;t<(times%4);++t) {
        int w=cur.Width(), h=cur.Height();
        ImageBuffer tmp(h, w, cur.Format(), cur.GetColorSpace());
        const uint8_t* s=cur.Data();
        uint8_t* d=tmp.Data();
        int bpp=BytesPerPixel(cur.Format());
        for (int y=0;y<h;++y)
            for (int x=0;x<w;++x)
                std::memcpy(d+((x)*(h)+(h-1-y))*bpp, s+(y*w+x)*bpp, bpp);
        cur = std::move(tmp);
    }
    return cur;
}

ImageBuffer CropTool::FlipH(const ImageBuffer& img) {
    ImageBuffer out = img.Clone();
    int w=out.Width(), h=out.Height();
    int bpp=BytesPerPixel(out.Format());
    uint8_t* d=out.Data();
    for (int y=0;y<h;++y)
        for (int x=0;x<w/2;++x) {
            uint8_t tmp[16];
            std::memcpy(tmp,   d+(y*w+x)*bpp, bpp);
            std::memcpy(d+(y*w+x)*bpp, d+(y*w+w-1-x)*bpp, bpp);
            std::memcpy(d+(y*w+w-1-x)*bpp, tmp, bpp);
        }
    return out;
}

ImageBuffer CropTool::FlipV(const ImageBuffer& img) {
    ImageBuffer out = img.Clone();
    out.FlipVertical();
    return out;
}

}  // namespace image
