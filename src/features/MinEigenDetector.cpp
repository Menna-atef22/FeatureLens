#include "MinEigenDetector.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace {
GrayImage blur(const GrayImage& img, int r, float sigma) {
    std::vector<float> k(2 * r + 1);
    float sum = 0.f;
    for (int i = -r; i <= r; ++i) { k[i + r] = std::exp(-(i * i) / (2.f * sigma * sigma)); sum += k[i + r]; }
    for (float& v : k) v /= sum;
    GrayImage tmp(img.width, img.height, 0.f), out(img.width, img.height, 0.f);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            float s = 0.f;
            for (int i = -r; i <= r; ++i) s += k[i + r] * img.at(std::clamp(x + i, 0, img.width - 1), y);
            tmp.at(x, y) = s;
        }
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            float s = 0.f;
            for (int i = -r; i <= r; ++i) s += k[i + r] * tmp.at(x, std::clamp(y + i, 0, img.height - 1));
            out.at(x, y) = s;
        }
    return out;
}
}

std::vector<Corner> MinEigenDetector::detect(const GrayImage& img, const MinEigenParams& p, double& elapsedMs) {
    auto t0 = std::chrono::high_resolution_clock::now();
    const int W = img.width, H = img.height;

    GrayImage Ixx(W, H, 0.f), Iyy(W, H, 0.f), Ixy(W, H, 0.f);
    for (int y = 1; y < H - 1; ++y)
        for (int x = 1; x < W - 1; ++x) {
            float gx = (-img.at(x-1,y-1) + img.at(x+1,y-1) - 2*img.at(x-1,y) + 2*img.at(x+1,y)
                        - img.at(x-1,y+1) + img.at(x+1,y+1)) / 8.f;
            float gy = (-img.at(x-1,y-1) - 2*img.at(x,y-1) - img.at(x+1,y-1)
                        + img.at(x-1,y+1) + 2*img.at(x,y+1) + img.at(x+1,y+1)) / 8.f;
            Ixx.at(x, y) = gx * gx; Iyy.at(x, y) = gy * gy; Ixy.at(x, y) = gx * gy;
        }
    GrayImage Sxx = blur(Ixx, p.gaussRadius, p.sigma);
    GrayImage Syy = blur(Iyy, p.gaussRadius, p.sigma);
    GrayImage Sxy = blur(Ixy, p.gaussRadius, p.sigma);

    // lambda- = (a + c)/2 - sqrt(((a - c)/2)^2 + b^2)
    GrayImage R(W, H, 0.f);
    float maxR = 0.f;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float a = Sxx.at(x, y), b = Sxy.at(x, y), c = Syy.at(x, y);
            float d = (a - c) * 0.5f;
            float l = (a + c) * 0.5f - std::sqrt(d * d + b * b);
            R.at(x, y) = l;
            maxR = std::max(maxR, l);
        }

    std::vector<Corner> corners;
    const float thr = p.thresholdRatio * maxR;
    const int n = p.nmsRadius;
    for (int y = n; y < H - n; ++y)
        for (int x = n; x < W - n; ++x) {
            float r = R.at(x, y);
            if (r <= thr) continue;
            bool isMax = true;
            for (int dy = -n; dy <= n && isMax; ++dy)
                for (int dx = -n; dx <= n; ++dx)
                    if ((dx || dy) && R.at(x + dx, y + dy) >= r) { isMax = false; break; }
            if (isMax) corners.push_back({x, y, r});
        }

    auto t1 = std::chrono::high_resolution_clock::now();
    elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return corners;
}
