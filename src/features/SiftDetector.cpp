#include "SiftDetector.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace {
const float kPi = 3.14159265358979f;

GrayImage gaussBlur(const GrayImage& img, float sigma) {
    const int r = std::max(1, int(std::ceil(3.f * sigma)));
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

GrayImage downsample(const GrayImage& img) {
    GrayImage out(std::max(1, img.width / 2), std::max(1, img.height / 2), 0.f);
    for (int y = 0; y < out.height; ++y)
        for (int x = 0; x < out.width; ++x) out.at(x, y) = img.at(2 * x, 2 * y);
    return out;
}

struct Octave {
    std::vector<GrayImage> gauss;   // s+3 images
    std::vector<GrayImage> dog;     // s+2 images
};

// pixel (x,y) of DoG level l is a 3x3x3 extremum?
bool isExtremum(const std::vector<GrayImage>& d, int l, int x, int y) {
    float v = d[l].at(x, y);
    bool mx = true, mn = true;
    for (int dl = -1; dl <= 1; ++dl)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if (!dl && !dy && !dx) continue;
                float o = d[l + dl].at(x + dx, y + dy);
                if (o >= v) mx = false;
                if (o <= v) mn = false;
                if (!mx && !mn) return false;
            }
    return mx || mn;
}

bool passesEdgeTest(const GrayImage& d, int x, int y, float r) {
    float dxx = d.at(x+1,y) + d.at(x-1,y) - 2*d.at(x,y);
    float dyy = d.at(x,y+1) + d.at(x,y-1) - 2*d.at(x,y);
    float dxy = (d.at(x+1,y+1) - d.at(x-1,y+1) - d.at(x+1,y-1) + d.at(x-1,y-1)) * 0.25f;
    float tr = dxx + dyy, det = dxx * dyy - dxy * dxy;
    if (det <= 0) return false;
    return tr * tr / det < (r + 1) * (r + 1) / r;
}

// dominant orientations of a keypoint on Gaussian image g (octave coordinates)
std::vector<float> orientations(const GrayImage& g, int x, int y, float sigmaOct) {
    const int N = 36;
    float hist[N] = {0};
    const float sw = 1.5f * sigmaOct;
    const int r = int(std::round(3.f * sw));
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx) {
            int px = x + dx, py = y + dy;
            if (px < 1 || py < 1 || px >= g.width - 1 || py >= g.height - 1) continue;
            float gx = g.at(px+1,py) - g.at(px-1,py), gy = g.at(px,py+1) - g.at(px,py-1);
            float mag = std::sqrt(gx*gx + gy*gy);
            float ang = std::atan2(gy, gx); if (ang < 0) ang += 2 * kPi;
            float w = std::exp(-(dx*dx + dy*dy) / (2.f * sw * sw));
            int b = int(ang / (2 * kPi) * N) % N;
            hist[b] += w * mag;
        }
    for (int it = 0; it < 2; ++it) {          // smooth histogram
        float t[N];
        for (int i = 0; i < N; ++i) t[i] = (hist[(i+N-1)%N] + hist[i] + hist[(i+1)%N]) / 3.f;
        std::copy(t, t + N, hist);
    }
    float mx = *std::max_element(hist, hist + N);
    std::vector<float> out;
    if (mx <= 0) return out;
    for (int i = 0; i < N; ++i) {
        float l = hist[(i+N-1)%N], c = hist[i], rr = hist[(i+1)%N];
        if (c > l && c > rr && c >= 0.8f * mx) {
            float off = 0.5f * (l - rr) / (l - 2*c + rr);       // parabolic peak interpolation
            out.push_back((i + 0.5f + off) * 2 * kPi / N);
        }
    }
    return out;
}

// 4x4 spatial cells x 8 orientation bins, trilinear interpolation
std::vector<float> descriptor(const GrayImage& g, float x, float y, float sigmaOct, float angle) {
    const int D = 4, B = 8;
    std::vector<float> hist(D * D * B, 0.f);
    const float cosA = std::cos(angle), sinA = std::sin(angle);
    const float cell = 3.f * sigmaOct;                       // width of one cell (octave px)
    const int radius = int(std::round(cell * std::sqrt(2.f) * (D + 1) * 0.5f));
    const float wsig = 0.5f * D;                             // gaussian weight over the window (in cells)
    for (int dy = -radius; dy <= radius; ++dy)
        for (int dx = -radius; dx <= radius; ++dx) {
            // rotate into the keypoint frame, express in cell units
            float rx = ( cosA * dx + sinA * dy) / cell;
            float ry = (-sinA * dx + cosA * dy) / cell;
            float cx = rx + D / 2.f - 0.5f, cy = ry + D / 2.f - 0.5f;
            if (cx <= -1 || cx >= D || cy <= -1 || cy >= D) continue;
            int px = int(std::round(x)) + dx, py = int(std::round(y)) + dy;
            if (px < 1 || py < 1 || px >= g.width - 1 || py >= g.height - 1) continue;
            float gx = g.at(px+1,py) - g.at(px-1,py), gy = g.at(px,py+1) - g.at(px,py-1);
            float mag = std::sqrt(gx*gx + gy*gy);
            float ang = std::atan2(gy, gx) - angle;
            while (ang < 0) ang += 2 * kPi;
            while (ang >= 2 * kPi) ang -= 2 * kPi;
            float ob = ang / (2 * kPi) * B;
            float w = mag * std::exp(-(rx*rx + ry*ry) / (2.f * wsig * wsig));
            int x0 = int(std::floor(cx)), y0 = int(std::floor(cy)), o0 = int(std::floor(ob));
            float fx = cx - x0, fy = cy - y0, fo = ob - o0;
            for (int iy = 0; iy < 2; ++iy)
                for (int ix = 0; ix < 2; ++ix) {
                    int xx = x0 + ix, yy = y0 + iy;
                    if (xx < 0 || xx >= D || yy < 0 || yy >= D) continue;
                    float ws = (ix ? fx : 1 - fx) * (iy ? fy : 1 - fy);
                    for (int io = 0; io < 2; ++io) {
                        int oo = (o0 + io) % B;
                        hist[(yy * D + xx) * B + oo] += w * ws * (io ? fo : 1 - fo);
                    }
                }
        }
    auto normalize = [&]() {
        float n = 0; for (float v : hist) n += v * v;
        n = std::sqrt(n) + 1e-7f;
        for (float& v : hist) v /= n;
    };
    normalize();
    for (float& v : hist) v = std::min(v, 0.2f);             // illumination robustness
    normalize();
    return hist;
}
} // namespace

std::vector<SiftKeypoint> SiftDetector::run(const GrayImage& img, const SiftParams& p,
                                            double& detectMs, double& describeMs) {
    using clk = std::chrono::high_resolution_clock;
    auto t0 = clk::now();
    const int S = p.scalesPerOctave;

    GrayImage base(img.width, img.height, 0.f);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) base.at(x, y) = img.at(x, y) / 255.f;
    base = gaussBlur(base, std::sqrt(std::max(0.01f, p.sigma0 * p.sigma0 - 0.25f)));

    // ---- build pyramid ----
    std::vector<Octave> pyr;
    const float k = std::pow(2.f, 1.f / S);
    GrayImage cur = base;
    for (int o = 0; o < p.octaves && cur.width >= 16 && cur.height >= 16; ++o) {
        Octave oc;
        oc.gauss.push_back(cur);
        float prev = p.sigma0;
        for (int i = 1; i < S + 3; ++i) {
            float total = p.sigma0 * std::pow(k, float(i));
            float inc = std::sqrt(total * total - prev * prev);
            oc.gauss.push_back(gaussBlur(oc.gauss.back(), inc));
            prev = total;
        }
        for (int i = 0; i + 1 < int(oc.gauss.size()); ++i) {
            GrayImage d(cur.width, cur.height, 0.f);
            for (int y = 0; y < cur.height; ++y)
                for (int x = 0; x < cur.width; ++x)
                    d.at(x, y) = oc.gauss[i+1].at(x, y) - oc.gauss[i].at(x, y);
            oc.dog.push_back(std::move(d));
        }
        cur = downsample(oc.gauss[S]);
        pyr.push_back(std::move(oc));
    }

    // ---- detect extrema ----
    struct Cand { int o, l, x, y; float resp; };
    std::vector<Cand> cands;
    const float thr = p.contrastThresh / S;
    for (int o = 0; o < int(pyr.size()); ++o) {
        const auto& d = pyr[o].dog;
        for (int l = 1; l + 1 < int(d.size()); ++l)
            for (int y = 2; y < d[l].height - 2; ++y)
                for (int x = 2; x < d[l].width - 2; ++x) {
                    float v = d[l].at(x, y);
                    if (std::fabs(v) < thr) continue;
                    if (!isExtremum(d, l, x, y)) continue;
                    if (!passesEdgeTest(d[l], x, y, p.edgeRatio)) continue;
                    cands.push_back({o, l, x, y, std::fabs(v)});
                }
    }
    std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.resp > b.resp; });
    if (p.maxKeypoints > 0 && int(cands.size()) > p.maxKeypoints) cands.resize(p.maxKeypoints);
    auto t1 = clk::now();
    detectMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // ---- orientation + descriptor ----
    std::vector<SiftKeypoint> out;
    for (const Cand& c : cands) {
        const GrayImage& g = pyr[c.o].gauss[c.l];
        const float sigmaOct = p.sigma0 * std::pow(k, float(c.l));
        const float scaleUp = float(1 << c.o);
        for (float ang : orientations(g, c.x, c.y, sigmaOct)) {
            SiftKeypoint kp;
            kp.x = (c.x + 0.5f) * scaleUp - 0.5f;
            kp.y = (c.y + 0.5f) * scaleUp - 0.5f;
            kp.scale = sigmaOct * scaleUp;
            kp.angle = ang;
            kp.response = c.resp;
            kp.desc = descriptor(g, float(c.x), float(c.y), sigmaOct, ang);
            out.push_back(std::move(kp));
        }
    }
    auto t2 = clk::now();
    describeMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    return out;
}
