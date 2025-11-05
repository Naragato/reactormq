#pragma once

#if defined(__has_include_next)
#  if __has_include_next(<netinet/in.h>)
#    include_next <netinet/in.h>
#  else
#    include <sys/socket.h>
#  endif
#else
#  include <sys/socket.h>
#endif