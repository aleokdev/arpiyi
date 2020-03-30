#ifndef ARPIYI_MATH_HPP
#define ARPIYI_MATH_HPP

namespace math {

struct IVec2D {
    int x, y;
};

struct Vec2D {
    float x, y;
};

struct Rect2D {
    Vec2D start;
    Vec2D end;
};

}

#endif // ARPIYI_MATH_HPP
