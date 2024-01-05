#ifndef TWICE_COMMON_MACROS_H
#define TWICE_COMMON_MACROS_H

#ifndef FORCE_INLINE
#  if defined(__clang__)
#    define FORCE_INLINE [[gnu::always_inline]] inline

#  elif defined(__GNUC__)
#    define FORCE_INLINE [[gnu::always_inline]] inline

#  elif defined(_MSC_VER)
#    define FORCE_INLINE __forceinline

#  else
#    define FORCE_INLINE inline
#  endif
#endif

#endif
