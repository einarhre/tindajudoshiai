JS_BUILD_DIR=/home/$(USER)/js-build
CC=gcc
LD=gcc

IS64 = $(shell uname -m | grep -o 64)
ifeq ($(IS64),64)
    PKGCONFIG=i686-linux-gnu-pkg-config
    GCCFLAG=-m32
    LDFLAG=-L/usr/lib/i386-linux-gnu
else
    PKGCONFIG=pkg-config
    GCCFLAG=
    LDFLAG=
endif

SUFF=
ZIP=
DEVELDIR=
OBJDIR=obj-linux32
RELEASEDIR=$(JS_BUILD_DIR)/release-linux32
TGT=LINUXOS

CFLAGS= -g $(WARNINGS) $(GCCFLAG) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags gtk+-$(GTKVER).0) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags cairo) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags librsvg-2.0) \
        $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags glib-2.0) \
        -DENABLE_BINRELOC -Wno-deprecated-declarations

LIBS=$(GCCFLAG) $(LDFLAG) $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --libs gtk+-$(GTKVER).0 gthread-2.0 cairo librsvg-2.0 glib-2.0) \
     $(shell curl-config --libs) -lssh2 -ldl

OBJS += $(OBJDIR)/binreloc.o
