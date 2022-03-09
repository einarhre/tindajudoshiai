#!/bin/bash

function report() {
    echo "**************************"
    echo "$1"
    echo "**************************"
}

cd
report "Get essentials"
sudo apt-get -y install build-essential libgtk-3-dev bison flex gettext
report "Get libraries"
sudo apt-get -y install librsvg2-dev libao-dev libmpg123-dev libcurl4-openssl-dev libssh2-1-dev
report "For JudoProxy:"
sudo apt-get -y install libavcodec-dev libavformat-dev libavresample-dev libavutil-dev libswscale-dev
sudo apt-get -y install liblzma-dev libxml2-dev
report "Git"
sudo apt-get -y install git
report "LibreOffice for PDF creation"
sudo apt-get -y install libreoffice
report "FPM for Debian package creation"
sudo apt-get -y install ruby ruby-dev rubygems
sudo -E gem install fpm
report "Stuff for WinXP build"
sudo apt-get -y install mingw32
sudo apt-get -y install wine
wget http://judoshiai.sourceforge.net/win32-gtk3.tgz
sudo tar xvzf win32-gtk3.tgz -C /opt
report "Stuff for Win32 and Win64 builds. This will take a long time to finish."
cd
git clone https://github.com/mxe/mxe.git
cd ~/mxe
make MXE_TARGETS='i686-w64-mingw32.shared x86_64-w64-mingw32.shared' cc gtk3 curl librsvg libao mpg123 gnutls
report "Get JudoShiai source code"
cd
git clone http://git.code.sf.net/p/judoshiai/judoshiai
cd judoshiai
report "Compile all"
./build-all.sh
