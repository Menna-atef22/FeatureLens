#pragma once
// ===== Person 1 : Harris Operator =====
#include <vector>
#include "common/Image.h"

struct HarrisParams {
    float k              = 0.04f;  // Harris constant (0.04 - 0.06)
    int   gaussRadius    = 2;      // window radius for Ix2, Iy2, Ixy
    float sigma          = 1.0f;   // gaussian sigma of the window
    float thresholdRatio = 0.01f;  // keep R > ratio * maxR
    int   nmsRadius      = 3;      // non-max suppression radius
};

class HarrisDetector {
public:
    // Returns detected corners; elapsedMs = computation time in milliseconds.
    static std::vector<Corner> detect(const GrayImage& img,
                                      const HarrisParams& p,
                                      double& elapsedMs);
private:
    static GrayImage sobelX(const GrayImage& img);
    static GrayImage sobelY(const GrayImage& img);
    static GrayImage multiply(const GrayImage& a, const GrayImage& b);
    static GrayImage gaussianBlur(const GrayImage& img, int radius, float sigma);
};
