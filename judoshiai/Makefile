#PROF=-pg
PROF=

ifeq ($(TARGETOS),WINXP)
  MATCH_TABLE=no
else
  MATCH_TABLE=yes
endif

include ../mk/common.mk

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

all: $(OBJDIR) $(OBJDIR)/judoshiai$(SUFF)

$(OBJDIR):
	mkdir -p $(OBJDIR)

OBJS += $(OBJDIR)/main.o $(OBJDIR)/competitors.o $(OBJDIR)/sheets.o $(OBJDIR)/set-one-judoka.o \
	$(OBJDIR)/sqlite3-patched.o $(OBJDIR)/popup-menu.o $(OBJDIR)/util.o $(OBJDIR)/db.o \
	$(OBJDIR)/categories.o $(OBJDIR)/log.o $(OBJDIR)/matches.o \
	$(OBJDIR)/names.o $(OBJDIR)/comm.o $(OBJDIR)/ip.o $(OBJDIR)/protocol.o \
	$(OBJDIR)/set-categories.o $(OBJDIR)/category-graph.o \
	$(OBJDIR)/completion.o $(OBJDIR)/another-shiai.o $(OBJDIR)/avl.o $(OBJDIR)/httpp.o \
	$(OBJDIR)/httpd.o $(OBJDIR)/import.o $(OBJDIR)/set-password.o \
	$(OBJDIR)/trees.o $(OBJDIR)/search.o $(OBJDIR)/menus.o \
	$(OBJDIR)/menuacts.o $(OBJDIR)/sql-dialog.o $(OBJDIR)/drawing.o $(OBJDIR)/print.o \
	$(OBJDIR)/print-texts.o $(OBJDIR)/db-competitors.o $(OBJDIR)/db-categories.o \
	$(OBJDIR)/db-matches.o $(OBJDIR)/db-table.o $(OBJDIR)/write-results.o \
	$(OBJDIR)/match-data.o $(OBJDIR)/serial.o \
	$(OBJDIR)/basic.tab.o $(OBJDIR)/lex.yy.o $(OBJDIR)/medal-matches.o $(OBJDIR)/db-validate.o \
	$(OBJDIR)/svg.o $(OBJDIR)/properties.o $(OBJDIR)/ftp.o $(OBJDIR)/language.o \
	$(OBJDIR)/custom-category-parser.o $(OBJDIR)/custom-category.o \
	$(OBJDIR)/custom-category-svg-template.o $(OBJDIR)/common.o $(OBJDIR)/sha1.o \
	$(OBJDIR)/websock.o $(OBJDIR)/websock-protocol.o  $(OBJDIR)/cJSON.o $(OBJDIR)/minilisp.o \
	$(OBJDIR)/svg-lisp.o $(OBJDIR)/auto-update.o $(OBJDIR)/common-utils.o \
	$(OBJDIR)/gdpr.o $(OBJDIR)/qrcode.o $(OBJDIR)/ws-broker.o $(OBJDIR)/ws-comm.o \
	$(OBJDIR)/unqlite.o $(OBJDIR)/ws-unqlite.o

ifeq ($(MATCH_TABLE),yes)
    OBJS += $(OBJDIR)/match-table.o
    CFLAGS += -DMATCH_TABLE=1
else
    OBJS += $(OBJDIR)/match-graph.o
endif

MHD_SRC := ../3rdparty/libmicrohttpd-latest.tar.gz

CFLAGS += -DAPPLICATION="\"judoshiai\""
CFLAGS += -DAPP_NUM=1
CFLAGS += -I../3rdparty/httpp -I../3rdparty/sqlite3 -I../3rdparty/cJSON -I../3rdparty/lisp \
	  -I../3rdparty/QRCode/src -I/usr/include/p11-kit-1 -I../3rdparty/microhttpd \
	  -I../3rdparty/unqlite \
	  -I$(OBJDIR)/wsbuild/include
	  #-I$(MHD_SRC)/../include

ifneq ($(TGT),WIN32OS)
LIBS += -lgnutls -lssl -lcrypto
endif

############################################
############ Windows executable ############
############################################

ifeq ($(TGT),WIN32OS)

ifeq ($(TOOL),CRS)
export PATH := $(CRSDIR)/bin:$(PATH)
else
export PATH := $(MXEDIR)/bin:$(PATH)
endif

OBJS += $(OBJDIR)/judoshiai-icon.res
ifeq ($(TOOL),CRS)
MHDLIB = $(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.a
else
MHDLIB = $(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.dll.a
endif

$(OBJDIR)/judoshiai-icon.res: judoshiai-icon.rc
	$(WINDRES) -O coff judoshiai-icon.rc $(OBJDIR)/judoshiai-icon.res

judoshiai-icon.rc: ../etc/judoshiai.ico
	echo "APPICONS ICON \"../etc/judoshiai.ico\"" >judoshiai-icon.rc

ifeq ($(TOOL),CRS)
MHDLIB = $(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.a
MHD_HOST = $(TRG)
MHD_PC_ENV = PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" PKG_CONFIG_PATH=""
#MHD_CONFIGURE_FLAGS = --host=$(MHD_HOST) --disable-shared --enable-static --with-gnutls --disable-examples --disable-doc
MHD_CONFIGURE_FLAGS = --host=$(MHD_HOST) --disable-shared --enable-static --without-gnutls --disable-https --disable-examples --disable-doc
else
MHDLIB = $(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.dll.a
MHD_HOST = $(MXE_CPU)-w64-mingw32.$(MXE_DYN)
MHD_PC_ENV = PKG_CONFIG_PATH=$(MXEDIR)/$(MHD_HOST)/lib/pkgconfig
MHD_CONFIGURE_FLAGS = --host=$(MHD_HOST) --with-gnutls --disable-examples --disable-doc
endif

$(MHDLIB):
	mkdir -p $(OBJDIR)/microhttpd
	tar xf $(MHD_SRC) -C $(OBJDIR)/microhttpd --strip-components 1
	cd $(OBJDIR)/microhttpd \
	&& $(MHD_PC_ENV) ./configure $(MHD_CONFIGURE_FLAGS) \
	&& make

$(OBJDIR)/ws-broker.o: ws-broker.c
	${CC} $(PROF) -g $(CFLAGS) -c ws-broker.c -o $@

############################################
############ Linux executable ##############
############################################

else # LINUXOS

MHDLIB = $(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.a
LIBS += -luv -lcap

PROF += -rdynamic

$(MHDLIB):
	mkdir -p $(OBJDIR)/microhttpd
	tar xf $(MHD_SRC) -C $(OBJDIR)/microhttpd --strip-components 1
	cd $(OBJDIR)/microhttpd && ./configure --disable-examples --disable-doc && make

$(OBJDIR)/ws-broker.o: ws-broker.c
	${CC} $(PROF) -g $(CFLAGS) -c ws-broker.c -o $@

endif

############################################
############ Common ########################
############################################

#$(OBJDIR)/judoshiai$(SUFF): $(OBJS) $(MHDLIB)
#	$(LD) $(PROF) -g $(RPATH) -o $(OBJDIR)/judoshiai$(SUFF) \
#	  -Wl,--start-group $(OBJS) $(MHDLIB) $(LIBS) -Wl,--end-group

$(OBJDIR)/judoshiai$(SUFF): $(OBJS) $(MHDLIB)
	$(LD) $(PROF) -g -Wl,-rpath -Wl,bin $(OBJS) $(MHDLIB) -o $(OBJDIR)/judoshiai$(SUFF) $(LIBS)

$(OBJDIR)/avl.o: ../common/avl.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/ip.o: ../common/ip.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/protocol.o: ../common/protocol.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/binreloc.o: ../common/binreloc.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/serial.o: ../common/serial.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/language.o: ../common/language.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/sha1.o: ../common/sha1.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c judoshiai.h ../common/comm.h
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/common.o: ../common/common.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/websock-protocol.o: ../common/websock-protocol.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/auto-update.o: ../common/auto-update.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/cJSON.o: ../3rdparty/cJSON/cJSON.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/httpp.o: ../3rdparty/httpp/httpp.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/unqlite.o: ../3rdparty/unqlite/unqlite.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/minilisp.o: ../3rdparty/lisp/minilisp.c
	${CC} -std=gnu99 $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/common-utils.o: ../common/common-utils.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/qrcode.o: ../3rdparty/QRCode/src/qrcode.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

sqlite3-patched.c: ../3rdparty/sqlite3/sqlite3.c sqlite3.patch
	cp ../3rdparty/sqlite3/sqlite3.c sqlite3-patched.c
	patch -p2 sqlite3-patched.c <sqlite3.patch

lex.yy.c: basic.l basic.tab.h
	flex basic.l

basic.tab.c basic.tab.h: basic.y
	bison -t -v -d basic.y

$(OBJDIR)/%.o: $(MHD_SRC)/%.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)/wsbuild
	rm -rf $(OBJDIR)/microhttpd
	rm -f $(OBJDIR)/* *.res *.rc *~ lex.yy.c basic.tab.c basic.tab.h
