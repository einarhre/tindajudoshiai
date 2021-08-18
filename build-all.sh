#!/bin/bash

LINUX32_IP=192.168.122.67
#RPI_IP=192.168.122.156
RPI_IP=192.168.129.139

mkdir -p ~/js-build/sourceforge/Android
mkdir -p ~/js-build/sourceforge/Linux-i386
mkdir -p ~/js-build/sourceforge/Linux-x86_64
mkdir -p ~/js-build/sourceforge/RaspberryPi
mkdir -p ~/js-build/sourceforge/Windows-32
mkdir -p ~/js-build/sourceforge/Windows-64
mkdir -p ~/js-build/sourceforge/WindowsXP


make_linux() {
    rm ~/js-build/sourceforge/Linux-x86_64/*
    date
    make clean && \
    make -j && \
    make debian && \
    make appimage && \
    mv ~/js-build/release-linux/*.deb ~/js-build/sourceforge/Linux-x86_64/ && \
    mv ~/js-build/release-linux/*.AppImage ~/js-build/sourceforge/Linux-x86_64/
}

make_win64() {
    rm ~/js-build/sourceforge/Windows-64/*
    date
    make clean TARGETOS=WIN64 && \
    make -j TARGETOS=WIN64       && \
    make setup TARGETOS=WIN64 && \
    mv ~/js-build/release-win64/*.exe ~/js-build/sourceforge/Windows-64/
}

make_win32() {
    rm ~/js-build/sourceforge/Windows-32/*
    date
    make clean TARGETOS=WIN32 && \
    make -j TARGETOS=WIN32       && \
    make setup TARGETOS=WIN32 && \
    mv ~/js-build/release-win32/*.exe ~/js-build/sourceforge/Windows-32/
}

make_remote_linux32() {
    # Linux 32 bit
    # Use external host (Mint in virtual machine)
    rm ~/js-build/sourceforge/Linux-i386/*
    date
    rsync -av * $LINUX32_IP:judoshiai/. && \
    ssh $LINUX32_IP "cd judoshiai && make clean && make && make debian && make appimage"
    scp $LINUX32_IP:js-build/release-linux32/*.{deb,AppImage} ~/js-build/sourceforge/Linux-i386/
}

make_remote_armhf() {
    # Raspberry Pi.
    # Use external host (virtual machine).
    # Remove etc/html/websock-serial since appimage do not accept i386 files.
    #
    # https://linuxconfig.org/how-to-run-the-raspberry-pi-os-in-a-virtual-machine-with-qemu-and-kvm
    # https://superuser.com/questions/24838/is-it-possible-to-resize-a-qemu-disk-image

    rm ~/js-build/sourceforge/RaspberryPi/*
    date
    rsync -av * pi@$RPI_IP:judoshiai/. && \
    ssh pi@$RPI_IP "cd judoshiai && rm etc/html/websock-serial && make clean && make && make debian && make appimage"
    scp pi@$RPI_IP:js-build/release-linux-armhf/*.{deb,AppImage} ~/js-build/sourceforge/RaspberryPi/
}

# Run compilations in parallel.
# Linux32 is compiled in virtual machine.
# Armhf is compiled in virtual Raspberry Pi.

WIN64="no"
WIN32="no"
LINUX64="no"
LINUX32="no"
RPI="no"

if [ "X$1" = "X" ] ; then
    echo "Usage: ./build-all.sh {all | win64 win32 linux linux32 rpi}"
fi

if [ "X$1" = "Xall" ] ; then
    WIN64="yes"
    WIN32="yes"
    LINUX64="yes"
    LINUX32="yes"
    RPI="yes"
fi

until [ -z "$1" ]  # Until all parameters used up . . .
do
    case "$1" in
        win64)
            WIN64="yes"
            ;;
        win32)
            WIN32="yes"
            ;;
        linux)
            LINUX64="yes"
            ;;
        linux32)
            LINUX32="yes"
            ;;
        rpi)
            RPI="yes"
            ;;
    esac
    shift
done

if [ "$LINUX32" = "yes" ]; then
    if ping -c 1 $LINUX32_IP ; then
        make_remote_linux32 &>> /tmp/build-linux32.log  &
    fi
fi

if [ "$RPI" = "yes" ]; then
    if ping -c 1 $RPI_IP ; then
        make_remote_armhf &>> /tmp/build-armhf.log &
    fi
fi

echo "Local compilation start. Monitor logfiles /tmp/build-*.log"

# These cannot be run in parallel (race conditions in common).

if [ "$LINUX64" = "yes" ]; then
    make_linux &>> /tmp/build-linux.log
fi

if [ "$WIN64" = "yes" ]; then
    make_win64 &>> /tmp/build-win64.log
fi

if [ "$WIN32" = "yes" ]; then
    make_win32 &>> /tmp/build-win32.log
fi

echo "Done. Check log files /tmp/build-*.log"
