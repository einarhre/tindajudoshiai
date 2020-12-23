TOOL=MINGW
JS_BUILD_DIR=/home/$(USER)/js-build
RELEASEDIR=$(JS_BUILD_DIR)/release-win64
MINGW_DIR="C:\\\\msys64"
OBJDIR=obj-win64
WINROOT=$(shell dirname $(shell dirname $(shell which gcc)))
DEVELDIR=$(WINROOT)
INNOSETUP1=$(shell find "/c/Program Files (x86)/." -name ISCC.exe)
INNOSETUP="$(INNOSETUP1)"
RESHACKER=$(WINROOT)/bin/ResHacker.exe
WINDRES=$(WINROOT)/bin/windres.exe
CC=gcc
LD=gcc
PKGCONFIG=pkg-config
TGT=WIN32OS
TOOL=MINGW
TGTEXT=64
SUFF=.exe
ZIP=.zip

CFLAGS= -g $(WARNINGS) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags gtk+-$(GTKVER).0) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags cairo) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags librsvg-2.0) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags glib-2.0) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags libmpg123) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags glib-2.0) \
        -Wno-deprecated-declarations \
        -mms-bitfields \
        -I$(DEVELDIR)/include

LIBS=$(shell $(PKGCONFIGPATH) $(PKGCONFIG) --libs gtk+-$(GTKVER).0 gthread-2.0 cairo librsvg-2.0 glib-2.0) \
     $(shell curl-config --libs) -lssh2 -lws2_32 -mwindows
     # -ldl
