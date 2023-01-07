#!/bin/bash

cd
git clone https://github.com/mxe/mxe.git
cd ~/mxe
make MXE_TARGETS='i686-w64-mingw32.shared x86_64-w64-mingw32.shared' gtk3 curl librsvg libao mpg123 pthreads libuv gnutls
