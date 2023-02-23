SHIAI_VER_NUM=4.0b
SHIAI_VERSION="\"$(SHIAI_VER_NUM)\""
LASTPART := $(shell basename $(CURDIR))
GTKVER=3
OBJS=
WARNINGS = -Wall -Wshadow
JUDOPROXY=NO
JUDOHTTPD=NO
LIBREOFFICE=libreoffice
FLUTTER=$(HOME)/src/flutter/bin/flutter

ifneq ("$(wildcard $(CURDIR)/judohttpd)","")
    JUDOHTTPD=NO
endif

ifeq ($(OS),Windows_NT)
  ifeq ($(TARGETOS),)
    IS64 = $(shell which gcc | grep -o 64)
    ifeq ($(IS64),64)
      TARGETOS=WIN64
    else
      TARGETOS=WIN32
    endif
  endif
  ifeq ($(TARGETOS),WIN32)
    -include ../mk/mingw32.mk
    -include mk/mingw32.mk
  endif
  ifeq ($(TARGETOS),WIN64)
    -include ../mk/mingw64.mk
    -include mk/mingw64.mk
  endif
  ifeq ($(TARGETOS),WINXP)
    -include ../mk/mingwxp.mk
    -include mk/mingwxp.mk
  endif
else
  ifeq ($(TARGETOS),)
    IS64 = $(shell uname -m | grep -o 64)
    ifeq ($(IS64),64)
      TARGETOS=LINUX
    else
      ISARM = $(shell uname -m | grep -o arm)
      ifeq ($(ISARM),arm)
        TARGETOS=LINUXARM
      else
        TARGETOS=LINUX32
      endif
    endif
  endif
  ifeq ($(TARGETOS),WIN32)
    -include ../mk/mxe-win32.mk
    -include mk/mxe-win32.mk
  endif
  ifeq ($(TARGETOS),WIN64)
    -include ../mk/mxe-win64.mk
    -include mk/mxe-win64.mk
  endif
  ifeq ($(TARGETOS),WINXP)
    -include ../mk/winxp.mk
    -include mk/winxp.mk
  endif
  ifeq ($(TARGETOS),LINUX)
    -include ../mk/linux.mk
    -include mk/linux.mk
  endif
  ifeq ($(TARGETOS),LINUX32)
    ARCHITECTURE=-a i386
    -include ../mk/linux32.mk
    -include mk/linux32.mk
  endif
  ifeq ($(TARGETOS),LINUXARM)
    -include ../mk/linux-armhf.mk
    -include mk/linux-armhf.mk
  endif
endif

TGTEXT_S = "\"$(TGTEXT)\""
RELDIR = $(RELEASEDIR)/judoshiai
CFLAGS += -I../common -DSHIAI_VERSION=$(SHIAI_VERSION) -DTGTEXT_S=$(TGTEXT_S)
CFLAGS += -DENABLE_NLS -DGTKVER=$(GTKVER) -DUSE_PANGO -DTARGETOS_$(TARGETOS)=1
