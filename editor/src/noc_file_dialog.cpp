#define NOC_FILE_DIALOG_IMPLEMENTATION
#if __linux__
#define NOC_FILE_DIALOG_GTK
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define NOC_FILE_DIALOG_WIN32
#elif __APPLE__
#define NOC_FILE_DIALOG_OSX
#elif __unix__
#define NOC_FILE_DIALOG_GTK
#else
#error "Could not determine target platform to use for file dialogs."
#endif

#include <noc_file_dialog.h>