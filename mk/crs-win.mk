# mk/crs-win.mk

TOOL = CRS
TGT = WIN32OS
SUFF = .exe
ZIP = .zip

ifeq ($(TARGETOS),WIN32)
  TGTEXT = 32
  CRS_CPU = i686
else ifeq ($(TARGETOS),WIN64)
  TGTEXT = 64
  CRS_CPU = x86_64
else
  $(error Unsupported CRS TARGETOS: $(TARGETOS))
endif

TRG = $(CRS_CPU)-w64-mingw32

CRS_ROOT ?= /home/eoh/src/CRS_BLD/install
WIN32_BASE ?= /home/eoh/src/PRG/tindajudoshiai/dev/win32
JS_BUILD_DIR ?= /home/eoh/src/PRG/tindajudoshiai/build

BUILD_KIND ?= static
override BUILD_KIND := $(strip $(BUILD_KIND))

ifeq ($(BUILD_KIND),dynamic)
  override BUILD_KIND := shared
endif

ifneq ($(filter $(BUILD_KIND),static shared),$(BUILD_KIND))
  $(error Unsupported BUILD_KIND: $(BUILD_KIND). Use static or shared)
endif

PREFIX = $(CRS_ROOT)/pkg/$(TRG)/$(BUILD_KIND)
DEVELDIR = $(PREFIX)
RUNDIR = $(DEVELDIR)

OBJDIR = obj-win$(TGTEXT)-$(BUILD_KIND)
RELEASEDIR = $(JS_BUILD_DIR)/release-win$(TGTEXT)-$(BUILD_KIND)
CRSDIR = $(CRS_ROOT)/cross/$(TRG)

override PROF :=
override RPATH :=

CC = $(CRSDIR)/bin/$(TRG)-gcc
LD = $(CC)
WINDRES = $(CRSDIR)/bin/$(TRG)-windres
OBJDUMP = $(CRSDIR)/bin/$(TRG)-objdump

PKG_CONFIG_LIBDIR = $(PREFIX)/lib/pkgconfig:$(PREFIX)/lib64/pkgconfig:$(PREFIX)/share/pkgconfig
PKG_CONFIG_PATH =

PC_ENV = PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" PKG_CONFIG_PATH=""
PKGCONFIG = $(PC_ENV) pkg-config

ifeq ($(BUILD_KIND),static)
  PC_STATIC = --static
  CURL_STATIC_DEFINE = -DCURL_STATICLIB
  LWS_PC = libwebsockets_static
else
  PC_STATIC =
  CURL_STATIC_DEFINE =
  LWS_PC = libwebsockets
endif

CFLAGS += -std=gnu17
CFLAGS += -I../common -DWIN32 -DTARGETOS_WIN32=1 -mms-bitfields
CFLAGS += $(CURL_STATIC_DEFINE)

CRS_PC_PKGS = gtk+-3.0 libcurl librsvg-2.0 libssh2 $(LWS_PC)
CRS_PC_PKGS += libxml-2.0 libcroco-0.6

CFLAGS += $(shell $(PKGCONFIG) --cflags $(CRS_PC_PKGS))

CRS_PC_LIBS := $(shell $(PKGCONFIG) $(PC_STATIC) --libs $(CRS_PC_PKGS))

ifeq ($(BUILD_KIND),static)
  CRS_PC_LIBS := $(filter-out -lstdc++,$(CRS_PC_LIBS))
endif

LIBS += $(CRS_PC_LIBS)
LIBS += -lhid -lws2_32 -lbcrypt -ladvapi32 -lcrypt32 -lole32 -luuid -lshlwapi

ifeq ($(BUILD_KIND),static)
  LIBS += -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic
  LIBS += -static-libgcc
  LIBS += -Wl,--allow-multiple-definition
endif

ifeq ($(TARGETOS),WIN64)
  CFLAGS += -Dg_libintl_gettext=libintl_gettext
  CFLAGS += -Dg_libintl_ngettext=libintl_ngettext
  CFLAGS += -Dg_libintl_bindtextdomain=libintl_bindtextdomain
  CFLAGS += -Dg_libintl_bind_textdomain_codeset=libintl_bind_textdomain_codeset
  CFLAGS += -Dg_libintl_textdomain=libintl_textdomain
endif
