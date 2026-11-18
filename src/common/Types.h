#pragma once
#include <cstdint>

struct Color3 { uint8_t r = 0, g = 0, b = 0; };

// A detected feature point (shared by Harris and Min-Eigenvalue)
struct Corner {
    int   x, y;
    float response;
};
