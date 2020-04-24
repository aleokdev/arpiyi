#include "util/process_exec.hpp"

namespace arpiyi::util {

#if defined(_WIN32) || defined(_WIN64)
#    include <windows.h>

void execute_process(fs::path const& path, std::string const& arg) {
    // From https://stackoverflow.com/questions/15435994/how-do-i-open-an-exe-from-another-c-exe
    // additional information
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    CreateProcess(fs::absolute(path).generic_wstring(), // the path
                  arg.c_str(),                          // Command line
                  NULL,                                 // Process handle not inheritable
                  NULL,                                 // Thread handle not inheritable
                  FALSE,                                // Set handle inheritance to FALSE
                  0,                                    // No creation flags
                  NULL,                                 // Use parent's environment block
                  NULL,                                 // Use parent's starting directory
                  &si,                                  // Pointer to STARTUPINFO structure
                  &pi // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );
    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
#elif defined(__linux__)
extern "C" {
#    include "stdlib.h"
}

void execute_process(fs::path const& path, std::string const& arg) {
    std::string file_path = fs::absolute(path).generic_string();
    system((file_path + " \"" + arg + "\"").c_str());
}

#endif

} // namespace arpiyi::util