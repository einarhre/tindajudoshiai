JS_BUILD_DIR=/home/$(USER)/js-build
WIN32_BASE=$(HOME)/Dropbox/judoshiai-dev/win32
WIN32_DIR=$(WIN32_BASE)
RELEASEDIR=$(JS_BUILD_DIR)/release-winxp
TGT=WIN32OS
TGTEXT=XP
TOOL=
SUFF=.exe
ZIP=.zip
RESHACKER=wine "C:\\bin\\ResHacker.exe"
INNOSETUP=wine "$(WIN32_BASE)/Inno Setup 5/ISCC.exe"
WINDRES=wine "$(WIN32_BASE)/MinGW/bin/windres.exe"
OBJDIR=obj-winxp

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
         -I../common -mms-bitfields -DWINXP=1

LIBS=-L$(DEVELDIR)/lib $(DEVELDIR)/lib/glib-2.0.lib -L$(RSVGDIR)/bin \
     -L$(DEVELDIR)/bin \
     -lusp10 \
     -lkernel32 -ladvapi32 \
     -lgtk-win32-$(GTKVER).0 -lgdk-win32-$(GTKVER).0 -latk-1.0 -lgio-2.0 -lgdk_pixbuf-2.0 \
     -lpangowin32-1.0 -lusp10 -lgdi32 -lpangocairo-1.0 -lpango-1.0 -lgobject-2.0 \
     -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lintl -lcairo \
     -lcroco-0.6-3 -lrsvg-2-2 -lxml2-2 \
     -L$(WIN32_BASE)/curl/bin -lcurl \
     -L$(WIN32_BASE)/ssh2/bin -lssh2-1 \
     -L$(WIN32_BASE)/w32api/lib -lws2_32 -mwindows
     # To add console out:
     # -mconsole
