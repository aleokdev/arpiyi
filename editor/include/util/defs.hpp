#ifndef ARPIYI_DEFS_HPP
#define ARPIYI_DEFS_HPP

#if defined(__builtin_unreachable)
#    define ARPIYI_UNREACHABLE() __builtin_unreachable()
#else
#    define ARPIYI_UNREACHABLE()
#endif

#endif // ARPIYI_DEFS_HPP
