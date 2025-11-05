#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<signal.h>)
#    include_next <signal.h>
#  else
#    include <sys/signal.h>
#  endif
#else
#  include <sys/signal.h>
#endif

#ifdef __cplusplus
extern "C"
{

#endif

int raise(int);

#ifdef __cplusplus
}
#endif