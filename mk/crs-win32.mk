TRG=i386_w64-mingw32
TOOL=CRS
TGTEXT=32
CRS_CPU=i686
TRG=$(CRS_CPU)-w64-mingw32

CRS_ROOT=/home/eoh/src/CRS_BLD/install
WIN32_BASE=/home/eoh/src/PRG/tindajudoshiai/dev/win32
JS_BUILD_DIR=/home/eoh/src/PRG/tindajudoshiai/build
RELEASEDIR=$(JS_BUILD_DIR)/release-win$(TGTEXT)
CRSDIR=$(CRS_ROOT)/cross/$(TRG)

BUILD_KIND ?= static
PREFIX=$(CRS_ROOT)/pkg/$(TRG)/$(BUILD_KIND)
DEVELDIR=$(PREFIX)

OBJDIR=obj-win$(TGTEXT)
SUFF=.exe
ZIP=.zip

#INNOSETUP=wine "$(WIN32_BASE)/Innosetup_6.0.5/app/ISCC.exe"

override PROF :=
override RPATH :=

CC=$(CRSDIR)/bin/$(TRG)-gcc
LD=$(CC)
WINDRES=$(CRSDIR)/bin/$(TRG)-windres

PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig:$(PREFIX)/lib64/pkgconfig:$(PREFIX)/share/pkgconfig
PKG_CONFIG_PATH=

PC_ENV=PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" PKG_CONFIG_PATH=""

ifeq ($(BUILD_KIND),static)
  PC_STATIC=--static
  CURL_STATIC_DEFINE=-DCURL_STATICLIB
  LWS_PC=libwebsockets_static
else
  PC_STATIC=
  CURL_STATIC_DEFINE=
  LWS_PC=libwebsockets
endif

CFLAGS += -std=gnu17
CFLAGS += -I../common -DWIN32 -DTARGETOS_WIN32=1 -mms-bitfields
CFLAGS += $(CURL_STATIC_DEFINE)

CFLAGS += $(shell $(PC_ENV) pkg-config --cflags gtk+-3.0)
CFLAGS += $(shell $(PC_ENV) pkg-config --cflags libcurl)
CFLAGS += $(shell $(PC_ENV) pkg-config --cflags librsvg-2.0)
CFLAGS += $(shell $(PC_ENV) pkg-config --cflags libssh2)
CFLAGS += $(shell $(PC_ENV) pkg-config --cflags $(LWS_PC))
CFLAGS += $(shell $(PC_ENV) pkg-config --cflags libxml-2.0 libcroco-0.6)

LIBS += $(shell $(PC_ENV) pkg-config $(PC_STATIC) --libs gtk+-3.0)
LIBS += $(shell $(PC_ENV) pkg-config $(PC_STATIC) --libs libcurl)
LIBS += $(shell $(PC_ENV) pkg-config $(PC_STATIC) --libs librsvg-2.0)
LIBS += $(shell $(PC_ENV) pkg-config $(PC_STATIC) --libs libssh2)
LIBS += $(shell $(PC_ENV) pkg-config $(PC_STATIC) --libs $(LWS_PC))
LIBS += $(shell $(PC_ENV) pkg-config $(PC_STATIC) --libs libxml-2.0 libcroco-0.6)

LIBS += -lhid -lws2_32 -lbcrypt -ladvapi32 -lcrypt32 -lole32 -luuid -lshlwapi
ifeq ($(BUILD_KIND),static)
  LIBS += -Wl,--allow-multiple-definition
endif
