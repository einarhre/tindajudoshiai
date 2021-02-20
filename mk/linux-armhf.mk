JS_BUILD_DIR=/home/$(USER)/js-build
CC=gcc
LD=gcc
PKGCONFIG=pkg-config
SUFF=
ZIP=
DEVELDIR=
OBJDIR=obj-linux-armhf
RELEASEDIR=$(JS_BUILD_DIR)/release-linux-armhf
TGT=LINUXOS

CFLAGS= -g $(WARNINGS) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags gtk+-$(GTKVER).0) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags cairo) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags librsvg-2.0) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags glib-2.0) \
        -DENABLE_BINRELOC -Wno-deprecated-declarations

LIBS=$(shell $(PKGCONFIGPATH) $(PKGCONFIG) --libs gtk+-$(GTKVER).0 gthread-2.0 cairo librsvg-2.0 glib-2.0) \
     $(shell curl-config --libs) -lssh2 -ldl

OBJS += $(OBJDIR)/binreloc.o
