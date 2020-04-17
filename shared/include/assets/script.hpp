#ifndef ARPIYI_SCRIPT_HPP
#define ARPIYI_SCRIPT_HPP

#include "asset.hpp"

#include <string>

namespace arpiyi::assets {

struct Script {
    std::string name;
    std::string source;
};

template<> struct LoadParams<Script> { fs::path path; };

template<> RawSaveData raw_get_save_data<Script>(Script const&);
template<> void raw_load<Script>(Script&, LoadParams<Script> const& params);

} // namespace arpiyi_editor::assets

#endif // ARPIYI_SCRIPT_HPP
