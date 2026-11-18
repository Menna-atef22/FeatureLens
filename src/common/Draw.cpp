#include "Draw.h"
#include <cmath>
#include <cstdlib>

static void put(ColorImage& img, int x, int y, Color3 c) {
    if (x >= 0 && y >= 0 && x < img.width && y < img.height) img.at(x, y) = c;
}

void drawCross(ColorImage& img, int x, int y, int r, Color3 c) {
    for (int i = -r; i <= r; ++i) { put(img, x + i, y, c); put(img, x, y + i, c); }
}

void drawCircle(ColorImage& img, int cx, int cy, int r, Color3 c) {
    const int steps = std::max(16, r * 8);
    for (int i = 0; i < steps; ++i) {
        double a = 2.0 * 3.14159265358979 * i / steps;
        put(img, cx + int(std::lround(r * std::cos(a))),
                 cy + int(std::lround(r * std::sin(a))), c);
    }
}

void drawLine(ColorImage& img, int x0, int y0, int x1, int y1, Color3 c) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        put(img, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
