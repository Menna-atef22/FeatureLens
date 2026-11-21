#include "Matcher.h"
#include <chrono>
#include <cmath>
#include <limits>

float Matcher::ssd(const std::vector<float>& u, const std::vector<float>& v) {
    float s = 0.f;
    for (size_t i = 0; i < u.size(); ++i) { float d = u[i] - v[i]; s += d * d; }
    return s;
}

float Matcher::ncc(const std::vector<float>& u, const std::vector<float>& v) {
    const size_t n = u.size();
    float mu = 0, mv = 0;
    for (size_t i = 0; i < n; ++i) { mu += u[i]; mv += v[i]; }
    mu /= n; mv /= n;
    float num = 0, du = 0, dv = 0;
    for (size_t i = 0; i < n; ++i) {
        float a = u[i] - mu, b = v[i] - mv;
        num += a * b; du += a * a; dv += b * b;
    }
    float den = std::sqrt(du * dv);
    return den > 1e-12f ? num / den : 0.f;
}

namespace {
// best (and second best) index in list L for descriptor d; "better" is lower for SSD, higher for NCC
void bestTwo(const std::vector<float>& d, const std::vector<SiftKeypoint>& L, bool useSsd,
             int& best, float& s1, float& s2) {
    best = -1;
    s1 = useSsd ? std::numeric_limits<float>::max() : -2.f;
    s2 = s1;
    for (int j = 0; j < int(L.size()); ++j) {
        float s = useSsd ? Matcher::ssd(d, L[j].desc) : Matcher::ncc(d, L[j].desc);
        bool better = useSsd ? s < s1 : s > s1;
        if (better) { s2 = s1; s1 = s; best = j; }
        else if (useSsd ? s < s2 : s > s2) s2 = s;
    }
}
}

std::vector<Match> Matcher::match(const std::vector<SiftKeypoint>& A, const std::vector<SiftKeypoint>& B,
                                  const MatchParams& p, double& elapsedMs) {
    auto t0 = std::chrono::high_resolution_clock::now();
    const bool useSsd = p.metric == MatchMetric::SSD;
    std::vector<Match> out;
    for (int i = 0; i < int(A.size()); ++i) {
        int j; float s1, s2;
        bestTwo(A[i].desc, B, useSsd, j, s1, s2);
        if (j < 0) continue;
        bool ok;
        if (useSsd) ok = B.size() < 2 || s2 <= 0.f || (s1 / s2) < p.ratio * p.ratio;   // SSD is squared distance
        else        ok = s1 >= p.nccThreshold;
        if (!ok) continue;
        if (p.mutual) {
            int back; float r1, r2;
            bestTwo(B[j].desc, A, useSsd, back, r1, r2);
            if (back != i) continue;
        }
        out.push_back({i, j, s1});
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return out;
}
