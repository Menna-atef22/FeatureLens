#include "ImageUtils.h"
#include <QPainter>
#include <QPen>
#include <algorithm>

ColorImage fromQImage(const QImage& src) {
    QImage q = src.convertToFormat(QImage::Format_RGB888);
    ColorImage out(q.width(), q.height());
    for (int y = 0; y < q.height(); ++y) {
        const uchar* row = q.constScanLine(y);
        for (int x = 0; x < q.width(); ++x)
            out.at(x, y) = { row[3*x], row[3*x + 1], row[3*x + 2] };
    }
    return out;
}

QImage toQImage(const ColorImage& img) {
    QImage q(img.width, img.height, QImage::Format_RGB888);
    for (int y = 0; y < img.height; ++y) {
        uchar* row = q.scanLine(y);
        for (int x = 0; x < img.width; ++x) {
            const Color3& c = img.at(x, y);
            row[3*x] = c.r; row[3*x + 1] = c.g; row[3*x + 2] = c.b;
        }
    }
    return q;
}

void drawCornerMarkers(QImage& img, const std::vector<Corner>& corners) {
    if (img.isNull() || corners.empty()) return;
    img = img.convertToFormat(QImage::Format_RGB32);

    const double maxDim = std::max(img.width(), img.height());
    const double r = std::max(10.0, maxDim / 55.0);     // ring radius (stays clear after "fit to window")
    const double w = std::max(3.0, r / 3.5);            // ring thickness
    const double d = std::max(2.5, w * 0.9);            // center dot radius
    const double o = std::max(3.0, w * 0.8);            // dark outline width

    const QColor halo (10, 10, 30, 235);                // dark outline = visible on any photo
    const QColor ring (255, 47, 160);                   // magenta ring
    const QColor dot  (255, 255, 255);                  // white center

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(Qt::NoBrush);
    auto at = [](const Corner& c) { return QPointF(c.x + 0.5, c.y + 0.5); };

    // three passes so neighbouring markers never cover each other's ring
    p.setPen(QPen(halo, w + o));
    for (const Corner& c : corners) p.drawEllipse(at(c), r, r);
    p.setPen(QPen(ring, w));
    for (const Corner& c : corners) p.drawEllipse(at(c), r, r);

    p.setPen(Qt::NoPen);
    for (const Corner& c : corners) {
        p.setBrush(halo); p.drawEllipse(at(c), d + 1.2, d + 1.2);
        p.setBrush(dot);  p.drawEllipse(at(c), d, d);
    }
}
