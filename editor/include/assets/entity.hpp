#ifndef ARPIYI_ENTITY_HPP
#define ARPIYI_ENTITY_HPP

#include "asset.hpp"
#include "asset_manager.hpp"
#include "sprite.hpp"

#include <sol/sol.hpp>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace arpiyi_editor::assets {

struct Entity {
    std::string name;
    Handle<Sprite> sprite;
    /// Position of this entity (measured in tiles).
    glm::vec2 pos;
    std::vector<sol::protected_function> scripts;
};

} // namespace arpiyi_editor::assets

#endif // ARPIYI_ENTITY_HPP
