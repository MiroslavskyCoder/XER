#include "color_matrix_filter.h"

namespace image {

ColorMatrixFilter ColorMatrixFilter::Grayscale() {
    return ColorMatrixFilter({{
        0.2126f, 0.7152f, 0.0722f, 0, 0,
        0.2126f, 0.7152f, 0.0722f, 0, 0,
        0.2126f, 0.7152f, 0.0722f, 0, 0,
        0,       0,       0,       1, 0
    }});
}

ColorMatrixFilter ColorMatrixFilter::Sepia(float a) {
    return ColorMatrixFilter({{
        0.393f*a+1-a, 0.769f*a,     0.189f*a,     0, 0,
        0.349f*a,     0.686f*a+1-a, 0.168f*a,     0, 0,
        0.272f*a,     0.534f*a,     0.131f*a+1-a, 0, 0,
        0, 0, 0, 1, 0
    }});
}

ColorMatrixFilter ColorMatrixFilter::Invert() {
    return ColorMatrixFilter({{
        -1, 0, 0, 0, 255,
         0,-1, 0, 0, 255,
         0, 0,-1, 0, 255,
         0, 0, 0, 1, 0
    }});
}

ColorMatrixFilter ColorMatrixFilter::Saturate(float s) {
    float r=0.213f, g=0.715f, b=0.072f;
    return ColorMatrixFilter({{
        r+(1-r)*s, g-g*s,     b-b*s,     0, 0,
        r-r*s,     g+(1-g)*s, b-b*s,     0, 0,
        r-r*s,     g-g*s,     b+(1-b)*s, 0, 0,
        0, 0, 0, 1, 0
    }});
}

void ColorMatrixFilter::Apply(ImageBuffer& img) const {
    if (!img.IsValid()) return;
    uint8_t* d = img.Data();
    int n = img.Width() * img.Height();
    const float* m = matrix_.data();
    for (int i = 0; i < n; ++i) {
        float r=d[i*4+0], g=d[i*4+1], b=d[i*4+2], a=d[i*4+3];
        auto cl=[](float v)->uint8_t{int x=static_cast<int>(v+0.5f);return static_cast<uint8_t>(x<0?0:x>255?255:x);};
        d[i*4+0]=cl(r*m[0] +g*m[1] +b*m[2] +a*m[3] +m[4]);
        d[i*4+1]=cl(r*m[5] +g*m[6] +b*m[7] +a*m[8] +m[9]);
        d[i*4+2]=cl(r*m[10]+g*m[11]+b*m[12]+a*m[13]+m[14]);
        d[i*4+3]=cl(r*m[15]+g*m[16]+b*m[17]+a*m[18]+m[19]);
    }
}

}  // namespace image
