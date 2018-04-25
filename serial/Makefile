include ../common/Makefile.inc

EXE = websock-serial$(SUFF)
APP = websock-serial-pkg$(ZIP)

OBJS = $(OBJDIR)/main.o $(OBJDIR)/serial.o $(OBJDIR)/comm.o \
       $(OBJDIR)/websock-protocol.o $(OBJDIR)/cJSON.o \
       $(OBJDIR)/websock.o $(OBJDIR)/httpp.o $(OBJDIR)/sha1.o \
       $(OBJDIR)/avl.o

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

CFLAGS += -I../3rdparty/httpp -I../3rdparty/sqlite3 -I../3rdparty/cJSON

$(OBJDIR)/$(APP): $(OBJDIR)/$(EXE)
ifeq ($(ZIP),.zip)
	zip -j $(OBJDIR)/$(APP) $(OBJDIR)/$(EXE) $(DLLS)
else
	mkdir -p $(OBJDIR)/$(APP)
	cp $(OBJDIR)/$(EXE) $(DLLS) $(OBJDIR)/$(APP)
endif

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJDIR) $(OBJS)
	$(LD) $(STATIC) -g $(OBJS) -o $(OBJDIR)/$(EXE) $(LIBS)
ifeq ($(TARGETOS),WIN32)
	cp $(DLLS) $(OBJDIR)
endif

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
