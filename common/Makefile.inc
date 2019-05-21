SHIAI_VER_NUM=2.9
SHIAI_VERSION="\"$(SHIAI_VER_NUM)\""

ifndef JS_BUILD_DIR
    ifeq ($(OS),Windows_NT)
        JS_BUILD_DIR=C:/js-build
    else
        JS_BUILD_DIR=/home/$(USER)/js-build
    endif
endif

LASTPART := $(shell basename $(CURDIR))
GTKVER=3
OBJS=
WARNINGS = -Wall -Wshadow
JUDOPROXY=NO
JUDOHTTPD=NO
TOOL=NORMAL
MXEDIR=/home/$(USER)/mxe/usr
TGTEXT_S="\"?\""

ifneq ("$(wildcard /opt/win32)","")
    WIN32_BASE=/opt/win32
else
    WIN32_BASE=$(HOME)/Dropbox/judoshiai-dev/win32
endif

ifneq ("$(wildcard $(CURDIR)/judohttpd)","")
    JUDOHTTPD=YES
endif

### Target executable type ###
ifeq ($(TARGETOS),WIN32)
    TGT=WIN32
    TOOL=MXE
    TGTEXT=32
    TGTEXT_S="\"$(TGTEXT)\""
else ifeq ($(TARGETOS),WIN64)
    TGT=WIN32
    TOOL=MXE
    TGTEXT=64
    TGTEXT_S="\"$(TGTEXT)\""
else ifeq ($(TARGETOS),WINXP)
    TGT=WIN32
    TGTEXT=XP
    TGTEXT_S="\"$(TGTEXT)\""
else
    ifeq ($(OS),Windows_NT)
        TGT=WIN32
        TGTEXT=NT
        TGTEXT_S="\"$(TGTEXT)\""
    else
        TGT=LINUX
        JUDOPROXY=YES
    endif
endif

####################################
# Default directories and programs #
####################################

ifeq ($(OS),Windows_NT)
### Windows (host=Windows, target=Windows) ###

    ifndef WIN32_DIR
        WIN32_DIR=/c/win32
    endif
    ifndef MINGW32_DIR
        MINGW32_DIR=/c/MinGW
    endif

    CC=gcc
    LD=gcc
    LIBREOFFICE=soffice.exe

    SUFF=.exe
    ZIP=
    DEVELDIR=$(WIN32_DIR)/gtk
    SOUNDDIR=$(WIN32_DIR)/mpg123
    RSVGDIR=$(WIN32_DIR)/rsvg
    CURLDIR=$(WIN32_DIR)/curl
    SSH2DIR=$(WIN32_DIR)/ssh2
    WEBKITDIR=$(WIN32_DIR)/webkitgtk
    SOAPDIR=$(WIN32_DIR)/soup
    RESHACKER=$(WIN32_DIR)/bin/ResHacker.exe
    WINDRES=$(MINGW32_DIR)/bin/windres.exe
    INNOSETUP="$(WIN32_DIR)/Inno Setup 5/ISCC.exe"
    OBJDIR=obj-win32
    RELEASEDIR=$(JS_BUILD_DIR)/release-win32

else
### Compilation in Linux ###

    ifeq ($(TGT),WIN32)
    # Common Windows definitions
        SUFF=.exe
        ZIP=.zip
        RESHACKER=wine "C:\\bin\\ResHacker.exe"
        INNOSETUP=wine "$(WIN32_BASE)/Inno Setup 5/ISCC.exe"
        LIBREOFFICE=libreoffice
    endif

    ifeq ($(TARGETOS),WINXP)
    ### Cross compilation (host=Linux, target=WindowsXP) ###

        WINDRES=wine "$(WIN32_BASE)/MinGW/bin/windres.exe"
        OBJDIR=obj-winxp
        RELEASEDIR=$(JS_BUILD_DIR)/release-winxp

        ifndef WIN32_DIR
            WIN32_DIR=$(WIN32_BASE)
        endif

        NEWCC := $(shell command -v i686-w64-mingw32-gcc 2> /dev/null)
        ifdef NEWCC
            CC=i686-w64-mingw32-gcc
            LD=i686-w64-mingw32-gcc
        else
            CC=i586-mingw32msvc-gcc
            LD=i586-mingw32msvc-gcc
        endif

        DEVELDIR=$(WIN32_DIR)/gtk
        SOUNDDIR=$(WIN32_DIR)/mpg123
        RSVGDIR=$(WIN32_DIR)/rsvg
        CURLDIR=$(WIN32_DIR)/curl
        SSH2DIR=$(WIN32_DIR)/ssh2
        WEBKITDIR=$(WIN32_DIR)/webkitgtk
        SOAPDIR=$(WIN32_DIR)/soup

    else ifeq ($(TOOL),MXE)
    ### Cross compilation (host=Linux, target=Windows32/64) ###

        ifeq ($(TARGETOS),WIN32)
            # 32 bit Windows
            OBJDIR=obj-win32
            RELEASEDIR=$(JS_BUILD_DIR)/release-win32
            MXE_CPU=i686
        else
            # 64 bit Windows
            OBJDIR=obj-win64
            RELEASEDIR=$(JS_BUILD_DIR)/release-win64
            MXE_CPU=x86_64
        endif

        WARNINGS += -Wjump-misses-init
        MXE_DYN=shared
        WIN32_DIR=$(MXEDIR)
        DEVELDIR=$(WIN32_DIR)/$(MXE_CPU)-w64-mingw32.$(MXE_DYN)
        PKGCONFIG = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-pkg-config
        CC = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-gcc
        LD = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-gcc
        WINDRES = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-windres

    else
    ### Linux ###

        CC=gcc
        LD=gcc
        PKGCONFIG=pkg-config
        LIBREOFFICE=libreoffice
        SUFF=
        ZIP=
        DEVELDIR=
        RESHACKER=
        WINDRES=
        INNOSETUP=
        OBJDIR=obj-linux
        RELEASEDIR=$(JS_BUILD_DIR)/release-linux
    # end Linux
    endif
endif

RELDIR=$(RELEASEDIR)/judoshiai

###################
# CFLAGS and LIBS #
###################

ifeq ($(OS),Windows_NT)
### Windows (host=Windows, target=Windows) ###

    # debug flags: -ggdb -gdwarf-2

    CFLAGS = $(WARNINGS) -g \
             -I$(DEVELDIR)/include/gtk-$(GTKVER).0 \
    -I$(DEVELDIR)/lib/gtk-$(GTKVER).0/include \
    -I$(DEVELDIR)/include/atk-1.0 \
    -I$(DEVELDIR)/include/cairo \
    -I$(DEVELDIR)/include/pango-1.0 \
    -I$(DEVELDIR)/include/glib-2.0 \
    -I$(DEVELDIR)/lib/glib-2.0/include \
    -I$(DEVELDIR)/include/freetype2 \
    -I$(DEVELDIR)/include \
    -I$(DEVELDIR)/include/libpng14     \
    -I$(DEVELDIR)/include/cairo \
    -I$(DEVELDIR)/include/gdk-pixbuf-2.0 \
    -I$(WIN32DIR)/w32api/include \
    -I$(RSVGDIR)/include/librsvg-2.0 \
    -I$(CURLDIR)/include \
    -I$(SSH2DIR)/include \
    -I../common -mms-bitfields \
    -DSHIAI_VERSION=$(SHIAI_VERSION) -DTGTEXT_S=$(TGTEXT_S) \
    -DENABLE_NLS \
    -DGTKVER=$(GTKVER) \
    -DUSE_PANGO \
    -Wno-deprecated-declarations

    ifeq ($(JUDOPROXY),YES)
        CFLAGS += -I$(WEBKITDIR)/include/webkitgtk-3.0 \
                  -I$(SOAPDIR)/include/libsoup-2.4
    endif

    LIBS=-L$(DEVELDIR)/lib $(DEVELDIR)/lib/glib-2.0.lib -L$(RSVGDIR)/bin \
    -L$(DEVELDIR)/bin \
    -lgtk-win32-$(GTKVER).0 -lgdk-win32-$(GTKVER).0 -latk-1.0 -lgio-2.0 -lgdk_pixbuf-2.0 \
    -lpangowin32-1.0 -lgdi32 -lpangocairo-1.0 -lpango-1.0 -lgobject-2.0 \
    -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lintl -lcairo \
    -lcroco-0.6-3 -lrsvg-2-2 -lxml2-2 \
    -L$(CURLDIR)/bin -lcurl -L$(SSH2DIR)/bin -lssh2-1 \
    -L$(WIN32DIR)/w32api/lib -lws2_32 -mwindows
    # To add console out:
    # -mconsole

    DLLS = $(DEVELDIR)/bin/libglib-2.0-0.dll $(DEVELDIR)/bin/libiconv-2.dll \
                 $(DEVELDIR)/bin/libintl-8.dll $(DEVELDIR)/bin/pthreadGC2.dll

    ifeq ($(JUDOPROXY),YES)
        LIBS += -L$(WEBKITDIR)/lib -lwebkitgtk-3.0.dll \
             -L$(SOAPDIR)/lib -lsoup-2.4.dll
    endif

else
### Cross compilation ###

    ifeq ($(TARGETOS),WINXP)
    # WindowsXP
        CFLAGS = $(WARNINGS) -g \
                 -I$(DEVELDIR)/include/gtk-$(GTKVER).0 \
                 -I$(DEVELDIR)/lib/gtk-$(GTKVER).0/include \
                 -I$(DEVELDIR)/include/atk-1.0 \
                 -I$(DEVELDIR)/include/cairo \
                 -I$(DEVELDIR)/include/pango-1.0 \
                 -I$(DEVELDIR)/include/glib-2.0 \
                 -I$(DEVELDIR)/lib/glib-2.0/include \
                 -I$(DEVELDIR)/include/freetype2 \
                 -I$(DEVELDIR)/include \
                 -I$(DEVELDIR)/include/libpng14   \
                 -I$(DEVELDIR)/include/cairo \
                 -I$(DEVELDIR)/include/gdk-pixbuf-2.0 \
                 -I$(WIN32_BASE)/w32api/include \
                 -I$(WIN32_BASE)/rsvg/include/librsvg-2.0 \
                 -I$(WIN32_BASE)/curl/include \
                 -I$(WIN32_BASE)/ssh2/include \
                 -I../common -mms-bitfields \
                 -DSHIAI_VERSION=$(SHIAI_VERSION) -DTGTEXT_S=$(TGTEXT_S) \
                 -DENABLE_NLS \
                 -DGTKVER=$(GTKVER) \
                 -DUSE_PANGO \
                 -Wno-deprecated-declarations

        LIBS=-L$(DEVELDIR)/lib $(DEVELDIR)/lib/glib-2.0.lib -L$(RSVGDIR)/bin \
             -L$(DEVELDIR)/bin \
             -lgtk-win32-$(GTKVER).0 -lgdk-win32-$(GTKVER).0 -latk-1.0 -lgio-2.0 -lgdk_pixbuf-2.0 \
             -lpangowin32-1.0 -lgdi32 -lpangocairo-1.0 -lpango-1.0 -lgobject-2.0 \
             -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lintl -lcairo \
             -lcroco-0.6-3 -lrsvg-2-2 -lxml2-2 \
             -L$(WIN32_BASE)/curl/bin -lcurl \
             -L$(WIN32_BASE)/ssh2/bin -lssh2-1 \
             -L$(WIN32_BASE)/w32api/lib -lws2_32 -mwindows
             # To add console out:
             # -mconsole

    else ifeq ($(TOOL),MXE)
    ### Windows32/64 using mxe tools ###
        CFLAGS = $(WARNINGS) -g \
                 -I$(DEVELDIR)/include/gtk-$(GTKVER).0 \
                 -I$(DEVELDIR)/include/atk-1.0 \
                 -I$(DEVELDIR)/include/cairo \
                 -I$(DEVELDIR)/include/pango-1.0 \
                 -I$(DEVELDIR)/include/glib-2.0 \
                 -I$(DEVELDIR)/lib/glib-2.0/include \
                 -I$(DEVELDIR)/include/freetype2 \
                 -I$(DEVELDIR)/include \
                 -I$(DEVELDIR)/include/libpng16   \
                 -I$(DEVELDIR)/include/cairo \
                 -I$(DEVELDIR)/include/gdk-pixbuf-2.0 \
                 -I$(DEVELDIR)/include/librsvg-2.0 \
                 -I../common -mms-bitfields \
                 -DSHIAI_VERSION=$(SHIAI_VERSION) -DTGTEXT_S=$(TGTEXT_S) \
                 -DENABLE_NLS \
                 -DGTKVER=$(GTKVER) \
                 -DUSE_PANGO \
                 -Wno-deprecated-declarations

        LIBS=$(shell $(PKGCONFIGPATH) $(PKGCONFIG) --libs gtk+-$(GTKVER).0 gthread-2.0 cairo librsvg-2.0 glib-2.0) \
             $(shell $(DEVELDIR)/bin/curl-config --libs) -lws2_32 -mwindows

    else
    ### Linux ###
        CFLAGS= -g $(WARNINGS) \
                $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags gtk+-$(GTKVER).0) \
                $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags cairo) \
                $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags librsvg-2.0) \
                $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --cflags glib-2.0) \
                -DENABLE_BINRELOC -DENABLE_NLS -DGTKVER=$(GTKVER) -DUSE_PANGO  \
                -Wno-deprecated-declarations \
                -I$(DEVELDIR)/include -I../common -DSHIAI_VERSION=$(SHIAI_VERSION) -DTGTEXT_S=$(TGTEXT_S)

        LIBS=$(shell $(PKGCONFIGPATH) $(PKGCONFIG) --libs gtk+-$(GTKVER).0 gthread-2.0 cairo librsvg-2.0 glib-2.0) \
             $(shell curl-config --libs) -lssh2 -ldl

        OBJS += $(OBJDIR)/binreloc.o

    endif
endif
