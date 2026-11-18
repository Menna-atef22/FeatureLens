#pragma once
#include <vector>
#include <cstddef>
#include "Types.h"

template <typename T>
class ImageT {
public:
    int width = 0, height = 0;

    ImageT() = default;
    ImageT(int w, int h, T init = T())
        : width(w), height(h), data_(static_cast<size_t>(w) * h, init) {}

    T&       at(int x, int y)       { return data_[static_cast<size_t>(y) * width + x]; }
    const T& at(int x, int y) const { return data_[static_cast<size_t>(y) * width + x]; }
    bool empty() const { return data_.empty(); }

private:
    std::vector<T> data_;
};

using GrayImage  = ImageT<float>;   // values in [0,255]
using ColorImage = ImageT<Color3>;

GrayImage toGray(const ColorImage& img);
