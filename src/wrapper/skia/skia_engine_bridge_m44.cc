#include "wrapper/skia/skia_engine_bridge_m44.h"
#include <cmath>

#if ENGINE_HAS_SKIA_BRIDGE
#include "include/core/SkM44.h"
#endif

namespace engine::bridge::skia {

#if ENGINE_HAS_SKIA_BRIDGE

static Matrix4x4 SkToStd(const SkM44& m) {
    Matrix4x4 res;
    m.getColMajor(res.data());
    return res;
}

static SkM44 StdToSk(const Matrix4x4& m) {
    return SkM44::ColMajor(m.data());
}

Matrix4x4 M44Identity() {
    return SkToStd(SkM44());
}

Matrix4x4 M44Translate(float x, float y, float z) {
    return SkToStd(SkM44::Translate(x, y, z));
}

Matrix4x4 M44Scale(float x, float y, float z) {
    return SkToStd(SkM44::Scale(x, y, z));
}

Matrix4x4 M44RotateX(float degrees) {
    float rad = degrees * (float)M_PI / 180.0f;
    return SkToStd(SkM44::Rotate({1, 0, 0}, rad));
}

Matrix4x4 M44RotateY(float degrees) {
    float rad = degrees * (float)M_PI / 180.0f;
    return SkToStd(SkM44::Rotate({0, 1, 0}, rad));
}

Matrix4x4 M44RotateZ(float degrees) {
    float rad = degrees * (float)M_PI / 180.0f;
    return SkToStd(SkM44::Rotate({0, 0, 1}, rad));
}

Matrix4x4 M44Multiply(const Matrix4x4& a, const Matrix4x4& b) {
    SkM44 ma = StdToSk(a);
    SkM44 mb = StdToSk(b);
    return SkToStd(ma * mb);
}

#else

Matrix4x4 M44Identity() { return {}; }
Matrix4x4 M44Translate(float x, float y, float z) { return {}; }
Matrix4x4 M44Scale(float x, float y, float z) { return {}; }
Matrix4x4 M44RotateX(float degrees) { return {}; }
Matrix4x4 M44RotateY(float degrees) { return {}; }
Matrix4x4 M44RotateZ(float degrees) { return {}; }
Matrix4x4 M44Multiply(const Matrix4x4& a, const Matrix4x4& b) { return {}; }

#endif

}
