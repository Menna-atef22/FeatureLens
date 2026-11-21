#pragma once
// ===== Min Eigenvalue (lambda-) / Shi-Tomasi corner detector =====
#include <vector>
#include "common/Image.h"

struct MinEigenParams {
    int   gaussRadius    = 2;      // window radius for Ix2, Iy2, Ixy
    float sigma          = 1.0f;   // gaussian sigma of the window
    float thresholdRatio = 0.01f;  // keep lambda- > ratio * max lambda-
    int   nmsRadius      = 3;      // non-max suppression radius
};

class MinEigenDetector {
public:
    // Returns detected corners; elapsedMs = computation time in milliseconds.
    static std::vector<Corner> detect(const GrayImage& img,
                                      const MinEigenParams& p,
                                      double& elapsedMs);
};
