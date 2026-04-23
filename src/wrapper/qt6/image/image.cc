#include "wrapper/qt6/image/image.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QImage>
#include <QImageReader>
#include <QString>
#include <QTransform>
#endif

namespace qt6::image {

ImageInfo GetInfo(const std::string& path) {
#if ENGINE_HAS_QT6
    QImageReader reader(QString::fromUtf8(path.c_str()));
    if (!reader.canRead()) return {};
    QSize sz = reader.size();
    ImageInfo info;
    info.width  = sz.width();
    info.height = sz.height();
    info.format = reader.format().toUpper().constData();
    info.valid  = true;
    // Load briefly for depth
    QImage img = reader.read();
    info.depth  = img.depth();
    return info;
#else
    return {};
#endif
}

bool Resize(const std::string& src, const std::string& dst,
            int width, int height, bool keep_aspect) {
#if ENGINE_HAS_QT6
    QImage img = LoadQImage(src);
    if (img.isNull()) return false;
    Qt::AspectRatioMode mode = keep_aspect
        ? Qt::KeepAspectRatio
        : Qt::IgnoreAspectRatio;
    QImage resized = img.scaled(width, height, mode, Qt::SmoothTransformation);
    return SaveQImage(resized, dst);
#else
    return false;
#endif
}

bool Convert(const std::string& src, const std::string& dst, int quality) {
#if ENGINE_HAS_QT6
    QImage img = LoadQImage(src);
    if (img.isNull()) return false;
    return SaveQImage(img, dst, quality);
#else
    return false;
#endif
}

bool Crop(const std::string& src, const std::string& dst,
          int x, int y, int width, int height) {
#if ENGINE_HAS_QT6
    QImage img = LoadQImage(src);
    if (img.isNull()) return false;
    QImage cropped = img.copy(x, y, width, height);
    return SaveQImage(cropped, dst);
#else
    return false;
#endif
}

bool FlipH(const std::string& src, const std::string& dst) {
#if ENGINE_HAS_QT6
    QImage img = LoadQImage(src);
    if (img.isNull()) return false;
    return SaveQImage(img.mirrored(true, false), dst);
#else
    return false;
#endif
}

bool FlipV(const std::string& src, const std::string& dst) {
#if ENGINE_HAS_QT6
    QImage img = LoadQImage(src);
    if (img.isNull()) return false;
    return SaveQImage(img.mirrored(false, true), dst);
#else
    return false;
#endif
}

bool Rotate(const std::string& src, const std::string& dst, int degrees) {
#if ENGINE_HAS_QT6
    QImage img = LoadQImage(src);
    if (img.isNull()) return false;
    QTransform t;
    t.rotate(degrees);
    return SaveQImage(img.transformed(t, Qt::SmoothTransformation), dst);
#else
    return false;
#endif
}

bool ToGrayscale(const std::string& src, const std::string& dst) {
#if ENGINE_HAS_QT6
    QImage img = LoadQImage(src);
    if (img.isNull()) return false;
    QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
    return SaveQImage(gray, dst);
#else
    return false;
#endif
}

#if ENGINE_HAS_QT6
QImage LoadQImage(const std::string& path) {
    return QImage(QString::fromUtf8(path.c_str()));
}

bool SaveQImage(const QImage& img, const std::string& path, int quality) {
    return img.save(QString::fromUtf8(path.c_str()), nullptr, quality);
}
#endif

}  // namespace qt6::image
