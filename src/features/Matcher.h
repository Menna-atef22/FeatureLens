#pragma once
// ===== Feature matching: SSD and normalized cross-correlation (NCC) =====
#include <vector>
#include "SiftDetector.h"

enum class MatchMetric { SSD, NCC };

struct MatchParams {
    MatchMetric metric   = MatchMetric::SSD;
    float ratio          = 0.8f;   // SSD: accept if best/second-best distance < ratio (Lowe ratio test)
    float nccThreshold   = 0.9f;   // NCC: accept if best correlation >= threshold
    bool  mutual         = false;  // also require the match to be the best in the reverse direction
};

struct Match {
    int   a, b;      // indices into the two keypoint lists
    float score;     // SSD (lower = better) or NCC (higher = better)
};

class Matcher {
public:
    static std::vector<Match> match(const std::vector<SiftKeypoint>& A,
                                    const std::vector<SiftKeypoint>& B,
                                    const MatchParams& p, double& elapsedMs);
    static float ssd(const std::vector<float>& u, const std::vector<float>& v);
    static float ncc(const std::vector<float>& u, const std::vector<float>& v);
};
