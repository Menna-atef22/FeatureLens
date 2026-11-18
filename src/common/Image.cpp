#include "Image.h"

GrayImage toGray(const ColorImage& img) {
    GrayImage out(img.width, img.height, 0.f);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            const Color3& c = img.at(x, y);
            out.at(x, y) = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
        }
    return out;
}
