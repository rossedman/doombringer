// Piecewise-linear curves for the character macros. Each macro sweeps several
// parameters, each along its own curve through a few breakpoints.
#pragma once

#include "Math.hpp"

namespace doom {

struct Breakpoint {
    float x, y;
};

// Piecewise-linear lookup; flat before the first and after the last point.
template <int N>
inline float curve(const Breakpoint (&pts)[N], float x) {
    if (x <= pts[0].x) return pts[0].y;
    for (int i = 1; i < N; ++i)
        if (x <= pts[i].x) return lerpf(pts[i - 1].y, pts[i].y, (x - pts[i - 1].x) / (pts[i].x - pts[i - 1].x));
    return pts[N - 1].y;
}

} // namespace doom
