MXEDIR=$(HOME)/src/mxe/usr
MXE_CPU=i686
MXE_DYN=shared
WIN32_DIR=$(MXEDIR)
JS_BUILD_DIR=$(HOME)/src/tindajudoshiai/tindajudoshiai-build
WIN32_BASE=$(HOME)/src/tindajudoshiai/tindajudoshiai-dev/win32
#MINGWDIR=/home/$(USER)/mingw32
TGT=WIN32OS
TOOL=MXE
TGTEXT=32
SUFF=.exe
ZIP=.zip
RESHACKER = wine "C:\\bin\\ResHacker.exe"
INNOSETUP=wine "$(WIN32_BASE)/Innosetup_6.0.5/app/ISCC.exe"
OBJDIR=obj-win32
RELEASEDIR=$(JS_BUILD_DIR)/release-win32
WARNINGS += -Wjump-misses-init
DEVELDIR=$(WIN32_DIR)/$(MXE_CPU)-w64-mingw32.$(MXE_DYN)
PKGCONFIG = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-pkg-config
CC = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-gcc
LD = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-gcc
WINDRES = PATH=$(WIN32_DIR)/bin:$(PATH) $(MXE_CPU)-w64-mingw32.$(MXE_DYN)-windres
TOOLCHAINFILE = mxe-win32.cmake

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
         -I../common -mms-bitfields \
         -Wno-deprecated-declarations

LIBS = -lusp10 -lws2_32 -mwindows \
	$(shell $(PKGCONFIGPATH) $(PKGCONFIG) \
	  --libs gtk+-$(GTKVER).0 gthread-2.0 cairo librsvg-2.0 glib-2.0 libwebsockets) \
    $(shell $(DEVELDIR)/bin/curl-config --libs) -lssh2
# To add console out:
# -mconsole
