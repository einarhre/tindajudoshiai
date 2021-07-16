#!/bin/bash

LINUX32_IP=192.168.99.104
RPI_IP=192.168.129.130

make_linux() {
    date
    make clean && \
    make -j && \
    make debian && \
    make appimage && \
    cp ~/js-build/release-linux/*.deb ~/js-build/ && \
    cp ~/js-build/release-linux/*.AppImage ~/js-build/
}

make_win64() {
    date
    make clean TARGETOS=WIN64 && \
    make -j TARGETOS=WIN64       && \
    make setup TARGETOS=WIN64 && \
    cp ~/js-build/release-win64/*.exe ~/js-build/
}

make_win32() {
    date
    make clean TARGETOS=WIN32 && \
    make -j TARGETOS=WIN32       && \
    make setup TARGETOS=WIN32 && \
    cp ~/js-build/release-win32/*.exe ~/js-build/
}

make_remote_linux32() {
    # Linux 32 bit
    # Use external host (Mint in virtual machine)
    date
    rsync -av * $LINUX32_IP:judoshiai/. && \
    ssh $LINUX32_IP "cd judoshiai && make clean && make && make debian && make appimage" && \
    scp $LINUX32_IP:js-build/release-linux32/*.{deb,AppImage} ~/js-build/.
}

make_remote_armhf() {
    # Raspberry Pi.
    # Use external host.
    # Remove etc/html/websock-serial since appimage do not accept i386 files.
    date
    rsync -av * pi@$RPI_IP:judoshiai/. && \
        ssh pi@$RPI_IP "cd judoshiai && rm etc/html/websock-serial && make clean && make && make debian && make appimage" && \
            scp pi@$RPI_IP:js-build/release-linux-armhf/*.{deb,AppImage} ~/js-build/.
}

# Run compilations in parallel.
# Linux32 is compiled in virtual machine.
# Armhf is compiled in real Raspberry Pi.

if ping -c 1 $LINUX32_IP ; then
    make_remote_linux32 &>> /tmp/build-linux32.log  &
fi

if ping -c 1 $RPI_IP ; then
    make_remote_armhf &>> /tmp/build-armhf.log &
fi

echo "Local compilation start. Monitor logfiles /tmp/build-*.log"

# These cannot be run in parallel (race conditions in common).
make_linux &>> /tmp/build-linux.log
make_win64 &>> /tmp/build-win64.log
make_win32 &>> /tmp/build-win32.log

echo "Done. Check log files /tmp/build-*.log"
