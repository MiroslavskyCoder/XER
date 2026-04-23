#include "wrapper/qt6/effects/filter.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QColor>
#include <QGraphicsBlurEffect>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QString>
#endif

namespace qt6::effects {

namespace {
#if ENGINE_HAS_QT6
// Render a QGraphicsItem with effect applied to get back a QImage
QImage ApplyGraphicsEffect(const QPixmap& pixmap, QGraphicsEffect* effect) {
    QGraphicsScene scene;
    QGraphicsPixmapItem* item = scene.addPixmap(pixmap);
    item->setGraphicsEffect(effect);

    QImage result(pixmap.width() + 40, pixmap.height() + 40,
                  QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    scene.render(&painter);
    painter.end();
    return result;
}
#endif
}  // namespace

bool ApplyBlur(const std::string& src, const std::string& dst,
               const BlurOptions& opts) {
#if ENGINE_HAS_QT6
    QImage img(QString::fromUtf8(src.c_str()));
    if (img.isNull()) return false;

    QGraphicsBlurEffect* effect = new QGraphicsBlurEffect();
    effect->setBlurRadius(opts.radius);
    effect->setBlurHints(opts.gaussian
        ? QGraphicsBlurEffect::QualityHint
        : QGraphicsBlurEffect::PerformanceHint);

    QImage result = ApplyGraphicsEffect(QPixmap::fromImage(img), effect);
    return result.save(QString::fromUtf8(dst.c_str()), "PNG");
#else
    return false;
#endif
}

bool ApplyDropShadow(const std::string& src, const std::string& dst,
                     const DropShadowOptions& opts) {
#if ENGINE_HAS_QT6
    QImage img(QString::fromUtf8(src.c_str()));
    if (img.isNull()) return false;

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setBlurRadius(opts.blur_radius);
    effect->setOffset(opts.offset_x, opts.offset_y);
    effect->setColor(QColor(QString::fromUtf8(opts.color.c_str())));

    QImage result = ApplyGraphicsEffect(QPixmap::fromImage(img), effect);
    return result.save(QString::fromUtf8(dst.c_str()), "PNG");
#else
    return false;
#endif
}

bool ApplyGlow(const std::string& src, const std::string& dst,
               const GlowOptions& opts) {
#if ENGINE_HAS_QT6
    // Glow = drop shadow with zero offset and glow color
    QImage img(QString::fromUtf8(src.c_str()));
    if (img.isNull()) return false;

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setBlurRadius(opts.radius);
    effect->setOffset(0, 0);
    effect->setColor(QColor(QString::fromUtf8(opts.color.c_str())));

    QImage result = ApplyGraphicsEffect(QPixmap::fromImage(img), effect);
    return result.save(QString::fromUtf8(dst.c_str()), "PNG");
#else
    return false;
#endif
}

bool AdjustBrightnessContrast(const std::string& src, const std::string& dst,
                               int brightness, int contrast) {
#if ENGINE_HAS_QT6
    QImage img(QString::fromUtf8(src.c_str()));
    if (img.isNull()) return false;

    // Convert to 32-bit for per-pixel manipulation
    img = img.convertToFormat(QImage::Format_ARGB32);

    const double b = brightness / 100.0;
    const double c = contrast  / 100.0;

    for (int y = 0; y < img.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            QRgb px  = line[x];
            int a    = qAlpha(px);
            auto adj = [&](int v) -> int {
                double fv = v / 255.0;
                fv = ((fv - 0.5) * c + 0.5) * b;
                return std::clamp(static_cast<int>(fv * 255.0 + 0.5), 0, 255);
            };
            line[x] = qRgba(adj(qRed(px)), adj(qGreen(px)), adj(qBlue(px)), a);
        }
    }

    return img.save(QString::fromUtf8(dst.c_str()), "PNG");
#else
    return false;
#endif
}

bool ApplyTint(const std::string& src, const std::string& dst,
               const std::string& color) {
#if ENGINE_HAS_QT6
    QImage img(QString::fromUtf8(src.c_str()));
    if (img.isNull()) return false;

    img = img.convertToFormat(QImage::Format_ARGB32);
    QColor tint(QString::fromUtf8(color.c_str()));

    const double tr = tint.redF();
    const double tg = tint.greenF();
    const double tb = tint.blueF();
    const double ta = tint.alphaF();

    for (int y = 0; y < img.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            QRgb px = line[x];
            line[x] = qRgba(
                static_cast<int>(qRed(px)   * tr),
                static_cast<int>(qGreen(px) * tg),
                static_cast<int>(qBlue(px)  * tb),
                static_cast<int>(qAlpha(px) * ta)
            );
        }
    }

    return img.save(QString::fromUtf8(dst.c_str()), "PNG");
#else
    return false;
#endif
}

}  // namespace qt6::effects
