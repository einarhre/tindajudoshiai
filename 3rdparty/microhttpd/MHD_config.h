#ifndef _MHD_CONFIG_H_
#define _MHD_CONFIG_H_

#if defined(__WIN32__) || defined(WIN32)

#include "MHD_config-win32.h"

#else

#include "MHD_config-linux.h"

#endif

#endif
