#!/bin/bash
make clean
make clean TARGETOS=WINXP
make clean TARGETOS=WIN32
make clean TARGETOS=WIN64
make -j && \
make -j TARGETOS=WINXP && \
make -j TARGETOS=WIN32 && \
make -j TARGETOS=WIN64 && \
make setup TARGETOS=WINXP && \
make setup TARGETOS=WIN32 && \
make setup TARGETOS=WIN64
