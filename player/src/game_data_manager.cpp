#include "game_data_manager.hpp"

namespace arpiyi::game_data_manager {

api::GamePlayData game_data;

api::GamePlayData& get_game_data() {
    return game_data;
}

}