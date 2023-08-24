#set(CROSS_PATH /home/hjokinen/mxe/usr/x86_64-w64-mingw32.shared)
set(CMAKE_FIND_ROOT_PATH /home/eoh/src/mxe/usr/x86_64-w64-mingw32.shared)

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
#set(CMAKE_C_COMPILER   x86_64-linux-gnu-gcc)
set(CMAKE_C_COMPILER /home/eoh/src/mxe/usr/bin/x86_64-w64-mingw32.shared-gcc)
#set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++)
set(CMAKE_CXX_COMPILER /home/eoh/src/mxe/usr/bin/x86_64-w64-mingw32.shared-g++)
#set(CMAKE_EXE_LINKER_FLAGS -w)
set(CMAKE_RC_COMPILER  /home/eoh/src/mxe/usr/bin/x86_64-w64-mingw32.shared-windres)

#set(LIBUV_LIBRARIES /home/hjokinen/mxe/usr/x86_64-w64-mingw32.shared/lib/libuv.la)
#include_directories(/home/hjokinen/mxe/usr/x86_64-w64-mingw32.shared/include)

#set(OPENSSL_CRYPTO_LIBRARY /home/hjokinen/mxe/usr/x86_64-w64-mingw32.shared/lib)
#set(OPENSSL_INCLUDE_DIR /home/hjokinen/mxe/usr/x86_64-w64-mingw32.shared/include)

# where is the target environment located
#set(CMAKE_FIND_ROOT_PATH  /usr/bin
#    /home/hjokinen/mingw-install)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
