#ifndef ARPIYI_MATH_HPP
#define ARPIYI_MATH_HPP

#include "util/intdef.hpp"
#include <anton/math/vector2.hpp>

namespace aml = anton::math;

namespace math {

struct IVec2D {
    i32 x, y;

    operator aml::Vector2() {
        return {static_cast<float>(x), static_cast<float>(y)};
    }
};

// TODO: Remove and use aml::Vector2 instead
struct Vec2D {
    float x, y;
};

struct Rect2D {
    Vec2D start;
    Vec2D end;
};

}

#endif // ARPIYI_MATH_HPP
