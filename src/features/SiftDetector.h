#pragma once
// ===== SIFT: DoG keypoints + 128-D descriptors =====
#include <vector>
#include "common/Image.h"

struct SiftParams {
    int   octaves          = 4;      // max number of octaves
    int   scalesPerOctave  = 3;      // s (DoG levels searched per octave)
    float sigma0           = 1.6f;   // base blur
    float contrastThresh   = 0.04f;  // |D| threshold (image in [0,1]), divided by s
    float edgeRatio        = 10.f;   // Lowe's r for edge rejection
    int   maxKeypoints     = 1500;   // keep strongest N (0 = unlimited)
};

struct SiftKeypoint {
    float x, y;          // position in ORIGINAL image coordinates
    float scale;         // sigma in original image coordinates
    float angle;         // dominant orientation, radians
    float response;      // |DoG| value (used to rank)
    std::vector<float> desc;   // 128 values, L2-normalised
};

class SiftDetector {
public:
    // Detects keypoints and computes descriptors. detectMs / describeMs are separate timings.
    static std::vector<SiftKeypoint> run(const GrayImage& img, const SiftParams& p,
                                         double& detectMs, double& describeMs);
};
