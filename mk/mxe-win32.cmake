set(CMAKE_FIND_ROOT_PATH /home/hjokinen/mxe/usr/i686-w64-mingw32.shared)

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
#set(CMAKE_C_COMPILER   i686-linux-gnu-gcc)
set(CMAKE_C_COMPILER /home/hjokinen/mxe/usr/bin/i686-w64-mingw32.shared-gcc)
#set(CMAKE_CXX_COMPILER i686-linux-gnu-g++)
set(CMAKE_CXX_COMPILER /home/hjokinen/mxe/usr/bin/i686-w64-mingw32.shared-g++)

set(CMAKE_RC_COMPILER  /home/hjokinen/mxe/usr/bin/i686-w64-mingw32.shared-windres)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
