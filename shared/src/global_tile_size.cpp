#include "global_tile_size.hpp"
#include <cassert>

namespace arpiyi::global_tile_size {

static u32 tile_size = -1;

u32 set(u32 size_in_px) {
    return tile_size = size_in_px;
}

u32 get() {
    assert(tile_size != -1);
    return tile_size;
}

}