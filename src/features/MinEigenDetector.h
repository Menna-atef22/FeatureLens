#pragma once
// ===== Person 2 : Min Eigenvalue (lambda-) / Shi-Tomasi =====
// TODO (Person 2): implement the detector here + MinEigenDetector.cpp
// Tip: reuse the same structure tensor as Harris, response = min(lambda1, lambda2)
//   lambda- = (a + c)/2 - sqrt(((a - c)/2)^2 + b^2)
#include <vector>
#include "common/Image.h"
