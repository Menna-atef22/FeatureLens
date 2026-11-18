#include "HarrisDetector.h"
#include <chrono>
#include <cmath>
#include <algorithm>

GrayImage HarrisDetector::sobelX(const GrayImage& img) {
    GrayImage out(img.width, img.height, 0.f);
    for (int y = 1; y < img.height - 1; ++y)
        for (int x = 1; x < img.width - 1; ++x) {
            float gx = -img.at(x-1,y-1) + img.at(x+1,y-1)
                     - 2*img.at(x-1,y)  + 2*img.at(x+1,y)
                     - img.at(x-1,y+1)  + img.at(x+1,y+1);
            out.at(x, y) = gx / 8.f;
        }
    return out;
}

GrayImage HarrisDetector::sobelY(const GrayImage& img) {
    GrayImage out(img.width, img.height, 0.f);
    for (int y = 1; y < img.height - 1; ++y)
        for (int x = 1; x < img.width - 1; ++x) {
            float gy = -img.at(x-1,y-1) - 2*img.at(x,y-1) - img.at(x+1,y-1)
                     +  img.at(x-1,y+1) + 2*img.at(x,y+1) + img.at(x+1,y+1);
            out.at(x, y) = gy / 8.f;
        }
    return out;
}

GrayImage HarrisDetector::multiply(const GrayImage& a, const GrayImage& b) {
    GrayImage out(a.width, a.height, 0.f);
    for (int y = 0; y < a.height; ++y)
        for (int x = 0; x < a.width; ++x)
            out.at(x, y) = a.at(x, y) * b.at(x, y);
    return out;
}

// Separable Gaussian blur = the summation window w(x,y)
GrayImage HarrisDetector::gaussianBlur(const GrayImage& img, int r, float sigma) {
    std::vector<float> k(2 * r + 1);
    float sum = 0.f;
    for (int i = -r; i <= r; ++i) {
        k[i + r] = std::exp(-(i * i) / (2.f * sigma * sigma));
        sum += k[i + r];
    }
    for (float& v : k) v /= sum;

    GrayImage tmp(img.width, img.height, 0.f), out(img.width, img.height, 0.f);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            float s = 0.f;
            for (int i = -r; i <= r; ++i)
                s += k[i + r] * img.at(std::clamp(x + i, 0, img.width - 1), y);
            tmp.at(x, y) = s;
        }
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            float s = 0.f;
            for (int i = -r; i <= r; ++i)
                s += k[i + r] * tmp.at(x, std::clamp(y + i, 0, img.height - 1));
            out.at(x, y) = s;
        }
    return out;
}

std::vector<Corner> HarrisDetector::detect(const GrayImage& img,
                                           const HarrisParams& p,
                                           double& elapsedMs) {
    auto t0 = std::chrono::high_resolution_clock::now();

    // 1) image gradients
    GrayImage Ix = sobelX(img), Iy = sobelY(img);

    // 2) structure tensor M = [Sxx Sxy; Sxy Syy]
    GrayImage Sxx = gaussianBlur(multiply(Ix, Ix), p.gaussRadius, p.sigma);
    GrayImage Syy = gaussianBlur(multiply(Iy, Iy), p.gaussRadius, p.sigma);
    GrayImage Sxy = gaussianBlur(multiply(Ix, Iy), p.gaussRadius, p.sigma);

    // 3) response  R = det(M) - k * trace(M)^2
    GrayImage R(img.width, img.height, 0.f);
    float maxR = 0.f;
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            float a = Sxx.at(x, y), b = Sxy.at(x, y), c = Syy.at(x, y);
            float det = a * c - b * b, tr = a + c;
            float r = det - p.k * tr * tr;
            R.at(x, y) = r;
            maxR = std::max(maxR, r);
        }

    // 4) threshold + non-maximum suppression
    std::vector<Corner> corners;
    float thr = p.thresholdRatio * maxR;
    int n = p.nmsRadius;
    for (int y = n; y < img.height - n; ++y)
        for (int x = n; x < img.width - n; ++x) {
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
