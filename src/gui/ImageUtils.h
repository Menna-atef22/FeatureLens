#pragma once
#include <QImage>
#include <vector>
#include "common/Image.h"

ColorImage fromQImage(const QImage& q);
QImage     toQImage(const ColorImage& img);

// Draws a clear marker on every detected point: a ring with a dark outline plus a
// center dot, sized relative to the image so it stays visible on big photos.
// Reusable by every tab (Harris, Min-Eigen, SIFT...).
void drawCornerMarkers(QImage& img, const std::vector<Corner>& corners);
