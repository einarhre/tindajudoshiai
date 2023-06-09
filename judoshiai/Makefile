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
	  -I../3rdparty/QRCode/src -I/usr/include/p11-kit-1 \
	  -I../3rdparty/unqlite \
	  -I$(OBJDIR)/wsbuild/include
	  #-I$(MHD_SRC)/../include

LIBS += -lgnutls -lssl -lcrypto

############################################
############ Windows executable ############
############################################

ifeq ($(TGT),WIN32OS)
export PATH := $(MXEDIR)/bin:$(PATH)

OBJS += $(OBJDIR)/judoshiai-icon.res
OBJS += $(OBJDIR)/wsbuild/lib/libwebsockets.dll.a
OBJS += $(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.dll.a

$(OBJDIR)/judoshiai-icon.res: judoshiai-icon.rc
	$(WINDRES) -O coff judoshiai-icon.rc $(OBJDIR)/judoshiai-icon.res

judoshiai-icon.rc: ../etc/judoshiai.ico
	echo "APPICONS ICON \"../etc/judoshiai.ico\"" >judoshiai-icon.rc

$(OBJDIR)/wsbuild/lib/libwebsockets.dll.a:
	mkdir -p $(OBJDIR)/wsbuild
	cmake ../3rdparty/libwebsockets -B $(OBJDIR)/wsbuild -DCMAKE_TOOLCHAIN_FILE=../../mk/$(TOOLCHAINFILE)
	#cmake -S ../3rdparty/libwebsockets -B $(OBJDIR)/wsbuild -DCMAKE_TOOLCHAIN_FILE=$(PWD)/../mk/$(TOOLCHAINFILE)
	#cp -r ../3rdparty/libwebsockets/wsbuild $(OBJDIR)/
	make -C $(OBJDIR)/wsbuild

$(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.dll.a:
	mkdir -p $(OBJDIR)/microhttpd
	tar xf $(MHD_SRC) -C $(OBJDIR)/microhttpd --strip-components 1
	PKG_CONFIG_PATH=$(MXEDIR)/$(MXE_CPU)-w64-mingw32.$(MXE_DYN)/lib/pkgconfig \
	cd $(OBJDIR)/microhttpd \
	&& ./configure --with-gnutls --host=$(MXE_CPU)-w64-mingw32.$(MXE_DYN) --disable-examples --disable-doc \
	&& make

CFLAGS += -I$(MINGW)/include

$(OBJDIR)/ws-broker.o: ws-broker.c $(OBJDIR)/wsbuild/lib/libwebsockets.dll.a
	${CC} $(PROF) -g $(CFLAGS) -c ws-broker.c -o $@

############################################
############ Linux executable ##############
############################################

else

OBJS += $(OBJDIR)/wsbuild/lib/libwebsockets.a
OBJS += $(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.a
LIBS += -luv -lcap

PROF += -rdynamic

$(OBJDIR)/wsbuild/lib/libwebsockets.a:
	mkdir -p $(OBJDIR)/wsbuild
	cmake ../3rdparty/libwebsockets -B $(OBJDIR)/wsbuild
	#cd $(OBJDIR)/wsbuild && echo PDW $(PWD) && cmake $(PWD)/../3rdparty/libwebsockets
	#cp -r ../3rdparty/libwebsockets/wsbuild $(OBJDIR)/
	make -C $(OBJDIR)/wsbuild

$(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd.a:
	mkdir -p $(OBJDIR)/microhttpd
	tar xf $(MHD_SRC) -C $(OBJDIR)/microhttpd --strip-components 1
	cd $(OBJDIR)/microhttpd && ./configure --disable-examples --disable-doc && make

$(OBJDIR)/ws-broker.o: ws-broker.c $(OBJDIR)/wsbuild/lib/libwebsockets.a
	${CC} $(PROF) -g $(CFLAGS) -c ws-broker.c -o $@

endif

############################################
############ Common ########################
############################################


$(OBJDIR)/judoshiai$(SUFF): $(OBJS)
	$(LD) $(PROF) -g -Wl,-rpath -Wl,bin $(OBJS) -o $(OBJDIR)/judoshiai$(SUFF) $(LIBS)

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
