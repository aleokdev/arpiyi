#ifndef ARPIYI_PROCESS_EXEC_HPP
#define ARPIYI_PROCESS_EXEC_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi::util {

void execute_process(fs::path const& path, std::string const& arg);

}

#endif // ARPIYI_PROCESS_EXEC_HPP
