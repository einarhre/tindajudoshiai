include ../mk/common.mk

EXE = websock-serial$(SUFF)
APP = websock-serial-pkg$(ZIP)

OBJS = $(OBJDIR)/main.o $(OBJDIR)/serial.o $(OBJDIR)/comm.o \
       $(OBJDIR)/websock-protocol.o $(OBJDIR)/cJSON.o \
       $(OBJDIR)/websock.o $(OBJDIR)/httpp.o $(OBJDIR)/sha1.o \
       $(OBJDIR)/avl.o

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

CFLAGS += -I../3rdparty/httpp -I../3rdparty/sqlite3 -I../3rdparty/cJSON
CFLAGS += -fPIC

ifeq ($(TGTEXT),XP)
DLLS = $(DEVELDIR)/bin/libglib-2.0-0.dll $(DEVELDIR)/bin/libiconv-2.dll \
       $(DEVELDIR)/bin/libintl-8.dll $(DEVELDIR)/bin/pthreadGC2.dll
endif

ifeq ($(TGTEXT),XP)
$(OBJDIR)/$(APP): $(OBJDIR)/$(EXE)
	7z a $(OBJDIR)/$(APP) $(OBJDIR)/$(EXE) $(DLLS)
else ifeq ($(TGT),LINUXOS)
$(OBJDIR)/$(APP): $(OBJDIR)/$(EXE)
	mkdir -p $(OBJDIR)/$(APP)
	cp $(OBJDIR)/$(EXE) $(OBJDIR)/$(APP)/
else
$(OBJDIR)/$(APP):
	-cp $(OBJDIR)/../obj-winxp/$(APP) $(OBJDIR)/
endif

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJDIR) $(OBJS)
	$(LD) $(STATIC) -g $(OBJS) -o $(OBJDIR)/$(EXE) $(LIBS)

$(OBJDIR)/%.o: %.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/avl.o: ../common/avl.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/websock-protocol.o: ../common/websock-protocol.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/websock.o: ../judoshiai/websock.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/httpp.o: ../3rdparty/httpp/httpp.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/cJSON.o: ../3rdparty/cJSON/cJSON.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/sha1.o: ../common/sha1.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o
