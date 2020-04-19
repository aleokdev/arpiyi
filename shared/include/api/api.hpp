#ifndef ARPIYI_API_HPP
#define ARPIYI_API_HPP

#include <glm/vec2.hpp>
#include <sol/sol.hpp>

namespace sol {

struct state_view;

}

namespace arpiyi::api {

struct GamePlayData;
class Camera {
public:
    explicit Camera(GamePlayData& data) noexcept : data(&data) {}
    glm::vec2 pos = {0,0};
    float zoom = 1;

private:
    GamePlayData * const data;
};

struct GamePlayData {
    GamePlayData() noexcept : cam(*this) {}

    Camera cam;
};

void define_api(sol::state_view const& s);

}

#endif // ARPIYI_API_HPP
