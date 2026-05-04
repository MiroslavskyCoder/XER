#include "displacement_map.h"
#include <vector>
#include <algorithm>
#include <cstring>

namespace image {

void DisplacementMap::Apply(ImageBuffer& img) const {
    if (!img.IsValid() || !map_.IsValid()) return;
    int w=img.Width(), h=img.Height();
    int mw=map_.Width(), mh=map_.Height();
    const uint8_t* src  = img.Data();
    const uint8_t* mdat = map_.Data();
    std::vector<uint8_t> out(static_cast<size_t>(w)*h*4, 0);
    for (int y=0;y<h;++y) {
        for (int x=0;x<w;++x) {
            int mx=std::min(x*mw/w, mw-1);
            int my=std::min(y*mh/h, mh-1);
            int mi=(my*mw+mx)*4;
            float dx=(mdat[mi+0]/127.5f-1.0f)*scale_;
            float dy=(mdat[mi+1]/127.5f-1.0f)*scale_;
            int sx=std::clamp(static_cast<int>(x+dx),0,w-1);
            int sy=std::clamp(static_cast<int>(y+dy),0,h-1);
            std::memcpy(&out[(y*w+x)*4], &src[(sy*w+sx)*4], 4);
        }
    }
    std::memcpy(img.Data(), out.data(), out.size());
}

}  // namespace image
