#ifndef ARPIYI_DEFS_HPP
#define ARPIYI_DEFS_HPP

#ifndef __has_builtin
#define __has_builtin(_) 0
#endif

#if __has_builtin(__builtin_unreachable)
#    define ARPIYI_UNREACHABLE() __builtin_unreachable()
#elif MSVC
#    define ARPIYI_UNREACHABLE() __assume(0)
#else
#    define ARPIYI_UNREACHABLE()
#endif

#endif // ARPIYI_DEFS_HPP
