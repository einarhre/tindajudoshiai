MXEDIR=/home/$(USER)/mxe/usr
MXE_CPU=x86_64
MXE_DYN=shared
MXE_BIN=$(MXEDIR)/$(MXE_CPU)-w64-mingw32.$(MXE_DYN)/bin
WIN32_DIR=$(MXEDIR)
JS_BUILD_DIR=/home/$(USER)/js-build
WIN32_BASE=$(HOME)/Dropbox/judoshiai-dev/win32
MINGWDIR=/home/$(USER)/mingw64
TGT=WIN32OS
TOOL=MXE
TGTEXT=64
SUFF=.exe
ZIP=.zip
RESHACKER = wine "C:\\bin\\ResHacker.exe"
INNOSETUP = wine "$(WIN32_BASE)/Inno Setup 5/ISCC.exe"
OBJDIR=obj-win64
RELEASEDIR=$(JS_BUILD_DIR)/release-win64
WARNINGS += -Wjump-misses-init
DEVELDIR=$(WIN32_DIR)/$(MXE_CPU)-w64-mingw32.$(MXE_DYN)
PKGCONFIG = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-pkg-config
CC = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-gcc
LD = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-gcc
WINDRES = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-windres
TOOLCHAINFILE = mxe-win64.cmake

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
         -I$(DEVELDIR)/include/harfbuzz \
         -I$(MINGWDIR)/include \
         -I../common -mms-bitfields \
         -Wno-deprecated-declarations

LIBS = -lusp10 -lws2_32 -mwindows \
	$(shell $(PKGCONFIGPATH) $(PKGCONFIG) --libs gtk+-$(GTKVER).0 gthread-2.0 cairo librsvg-2.0 glib-2.0) \
        $(shell $(DEVELDIR)/bin/curl-config --libs) -lssh2 \
        -L$(MINGWDIR)/lib
# To add console out:
# -mconsole
