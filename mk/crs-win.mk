# mk/crs-win.mk
#
# Common CRS Windows cross-build settings for both WIN32 and WIN64.
#
# BUILD_KIND:
#   static  - link CRS-built third-party libraries statically where possible
#   shared  - use CRS shared libraries and copy DLLs to the release tree
#
# Windows subsystem:
#   GUI applications can set:
#
#       WINDOWS_GUI = YES
#
#   in their own Makefile.  This adds -mwindows at link time, which prevents
#   Windows from opening a console window before starting the GTK GUI.
#
#   The default is console, because command-line tools such as db-convert.exe
#   should keep stdout/stderr and a normal console subsystem.

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

ifeq ($(BUILD_KIND),shared)
  ifeq ($(TARGETOS),WIN32)
    CRS_GCC_RUNTIME_DLL = libgcc_s_sjlj-1.dll
  else
    CRS_GCC_RUNTIME_DLL = libgcc_s_seh-1.dll
  endif

  # GCC runtime DLLs live in the GCC runtime library directory.
  CRS_GCC_RUNTIME_DLLS = $(foreach dll,$(CRS_GCC_RUNTIME_DLL) libstdc++-6.dll,$(shell $(CC) -print-file-name=$(dll)))
  CRS_GCC_RUNTIME_DLLS := $(filter /%,$(CRS_GCC_RUNTIME_DLLS))

  # libwinpthread-1.dll lives in the target bin directory, not where
  # gcc -print-file-name finds libgcc/libstdc++.
  CRS_TARGET_RUNTIME_DLLS = $(wildcard $(CRSDIR)/$(TRG)/bin/libwinpthread-1.dll)

  # GCC/MinGW runtime DLLs that live outside $(DEVELDIR)/bin.
  CRS_RUNTIME_DLLS = $(CRS_GCC_RUNTIME_DLLS) $(CRS_TARGET_RUNTIME_DLLS)

  # libmicrohttpd is built locally under the judoshiai object tree.  It is
  # copied by the release recipe with shell wildcard expansion, because it may
  # not exist when make expands variables for the recipe.
  CRS_LOCAL_RUNTIME_DLL_DIR = $(JS_BUILD_DIR)/judoshiai/$(OBJDIR)/microhttpd/src/microhttpd/.libs
endif

ifeq ($(BUILD_KIND),static)
  PC_STATIC = --static
  CURL_STATIC_DEFINE = -DCURL_STATICLIB
  LWS_PC = libwebsockets_static
else
  PC_STATIC =
  CURL_STATIC_DEFINE =
  LWS_PC = libwebsockets
endif

# Most source code still tests WIN32 for the Windows API, even for 64-bit
# builds. TARGETOS_WIN32 is kept for compatibility with the existing code.
CFLAGS += -std=gnu17
CFLAGS += -I../common -DWIN32 -DTARGETOS_WIN32=1 -mms-bitfields
CFLAGS += $(CURL_STATIC_DEFINE)

CRS_PC_PKGS = gtk+-3.0 libcurl librsvg-2.0 libssh2 $(LWS_PC)
CRS_PC_PKGS += libxml-2.0 libcroco-0.6

CFLAGS += $(shell $(PKGCONFIG) --cflags $(CRS_PC_PKGS))

CRS_PC_LIBS := $(shell $(PKGCONFIG) $(PC_STATIC) --libs $(CRS_PC_PKGS))

ifeq ($(BUILD_KIND),static)
  # pkg-config --static may pull in -lstdc++.  If left untouched, MinGW can
  # choose libstdc++-6.dll.  Remove it here and add it explicitly below inside
  # a -Bstatic/-Bdynamic pair.
  #
  # The x86_64 static libcurl.pc currently also emits -lnetio, which creates
  # an invalid user-mode runtime import on NETIO.SYS.  User-mode networking
  # should use normal Windows DLLs such as WS2_32/IPHLPAPI, not NETIO.SYS.
  CRS_PC_LIBS := $(filter-out -lstdc++ -lnetio,$(CRS_PC_LIBS))
endif

# GUI programs may set WINDOWS_GUI=YES in their own Makefile.  Keep this as a
# recursively-expanded variable so the application Makefile may set it either
# before or after including the common makefiles.
WINDOWS_GUI ?= NO
WIN_GUI_LDFLAG = $(if $(filter YES yes 1 true TRUE,$(WINDOWS_GUI)),-mwindows,)

LIBS += $(CRS_PC_LIBS)
LIBS += -lhid -lws2_32 -liphlpapi -lbcrypt -ladvapi32 -lcrypt32 -lole32 -luuid -lshlwapi
LIBS += $(WIN_GUI_LDFLAG)

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
