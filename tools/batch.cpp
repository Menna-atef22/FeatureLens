// Headless batch tool (no Qt): runs one algorithm on PPM images and writes a PPM result.
// Used by tools/run_all.py to generate every result image + timing.
//   FeatureLensBatch harris|mineig|sift  in.ppm out.ppm [gray]
//   FeatureLensBatch match  a.ppm b.ppm out.ppm ssd|ncc
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "common/Draw.h"
#include "common/Image.h"
#include "features/HarrisDetector.h"
#include "features/MinEigenDetector.h"
#include "features/Matcher.h"
#include "features/SiftDetector.h"

static bool readPPM(const char* path, ColorImage& img) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    char magic[3] = {0}; int w, h, mx;
    auto skip = [&]() { int c; while ((c = std::fgetc(f)) == '#') while ((c = std::fgetc(f)) != '\n' && c != EOF) {} std::ungetc(c, f); };
    if (std::fscanf(f, "%2s", magic) != 1 || std::strcmp(magic, "P6")) { std::fclose(f); return false; }
    skip(); if (std::fscanf(f, "%d", &w) != 1) return false;
    skip(); if (std::fscanf(f, "%d", &h) != 1) return false;
    skip(); if (std::fscanf(f, "%d", &mx) != 1) return false;
    std::fgetc(f);
    img = ColorImage(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            unsigned char p[3];
            if (std::fread(p, 1, 3, f) != 3) { std::fclose(f); return false; }
            img.at(x, y) = {p[0], p[1], p[2]};
        }
    std::fclose(f);
    return true;
}

static bool writePPM(const char* path, const ColorImage& img) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", img.width, img.height);
    for (int y = 0; y < img.height; ++y)
        for (int x = 0; x < img.width; ++x) {
            const Color3& c = img.at(x, y);
            unsigned char p[3] = {c.r, c.g, c.b};
            std::fwrite(p, 1, 3, f);
        }
    std::fclose(f);
    return true;
}

static ColorImage grayToColor(const GrayImage& g) {
    ColorImage o(g.width, g.height);
    for (int y = 0; y < g.height; ++y)
        for (int x = 0; x < g.width; ++x) {
            uint8_t v = uint8_t(std::clamp(g.at(x, y), 0.f, 255.f) + 0.5f);
            o.at(x, y) = {v, v, v};
        }
    return o;
}

static void marker(ColorImage& img, int x, int y) {
    const int r = std::max(3, std::max(img.width, img.height) / 170);
    const Color3 dark{10, 10, 30}, ring{255, 47, 160};
    drawCircle(img, x, y, r + 1, dark);
    drawCircle(img, x, y, r, ring);
}

static Color3 hue(int i) {
    float h = float((i * 47) % 360) / 60.f, f = h - std::floor(h);
    float q = 1 - f; int k = int(h) % 6;
    float r[6] = {1, q, 0, 0, f, 1}, g[6] = {f, 1, 1, q, 0, 0}, b[6] = {0, 0, f, 1, 1, q};
    return {uint8_t(r[k] * 255), uint8_t(g[k] * 255), uint8_t(b[k] * 255)};
}

int main(int argc, char** argv) {
    if (argc < 4) { std::fprintf(stderr, "usage: see header of batch.cpp\n"); return 2; }
    const std::string cmd = argv[1];

    if (cmd == "match") {
        if (argc < 6) return 2;
        ColorImage A, B;
        if (!readPPM(argv[2], A) || !readPPM(argv[3], B)) { std::fprintf(stderr, "read error\n"); return 1; }
        double d1, e1, d2, e2, mm;
        auto ka = SiftDetector::run(toGray(A), SiftParams(), d1, e1);
        auto kb = SiftDetector::run(toGray(B), SiftParams(), d2, e2);
        MatchParams mp; mp.metric = std::string(argv[5]) == "ncc" ? MatchMetric::NCC : MatchMetric::SSD;
        auto m = Matcher::match(ka, kb, mp, mm);

        ColorImage out(A.width + B.width, std::max(A.height, B.height), Color3{20, 20, 30});
        for (int y = 0; y < A.height; ++y) for (int x = 0; x < A.width; ++x) out.at(x, y) = A.at(x, y);
        for (int y = 0; y < B.height; ++y) for (int x = 0; x < B.width; ++x) out.at(x + A.width, y) = B.at(x, y);
        for (size_t i = 0; i < m.size(); ++i) {
            Color3 c = hue(int(i));
            int x0 = int(ka[m[i].a].x + .5f), y0 = int(ka[m[i].a].y + .5f);
            int x1 = int(kb[m[i].b].x + .5f) + A.width, y1 = int(kb[m[i].b].y + .5f);
            drawLine(out, x0, y0, x1, y1, c);
            drawCircle(out, x0, y0, 3, c); drawCircle(out, x1, y1, 3, c);
        }
        writePPM(argv[4], out);
        std::printf("%zu %zu %zu %.3f %.3f\n", ka.size(), kb.size(), m.size(), d1 + e1 + d2 + e2, mm);
        return 0;
    }

    ColorImage img;
    if (!readPPM(argv[2], img)) { std::fprintf(stderr, "read error\n"); return 1; }
    const bool gray = argc > 4 && std::string(argv[4]) == "gray";
    GrayImage g = toGray(img);
    ColorImage out = gray ? grayToColor(g) : img;

    if (cmd == "harris" || cmd == "mineig") {
        double ms;
        std::vector<Corner> c = cmd == "harris" ? HarrisDetector::detect(g, HarrisParams(), ms)
                                                : MinEigenDetector::detect(g, MinEigenParams(), ms);
        for (const Corner& k : c) marker(out, k.x, k.y);
        writePPM(argv[3], out);
        std::printf("%zu %.3f\n", c.size(), ms);
    } else if (cmd == "sift") {
        double det, desc;
        auto kp = SiftDetector::run(g, SiftParams(), det, desc);
        for (const auto& k : kp) {
            marker(out, int(k.x + .5f), int(k.y + .5f));
            int r = std::max(3, int(k.scale * 3));          // scale + orientation line
            drawLine(out, int(k.x), int(k.y), int(k.x + r * std::cos(k.angle)), int(k.y + r * std::sin(k.angle)), Color3{0, 255, 120});
        }
        writePPM(argv[3], out);
        std::printf("%zu %.3f %.3f\n", kp.size(), det, desc);
    } else return 2;
    return 0;
}
