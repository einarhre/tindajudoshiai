include ../mk/common.mk

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

all: $(OBJDIR) $(OBJDIR)/judoweight$(SUFF)

$(OBJDIR):
	mkdir -p $(OBJDIR)

OBJS += $(OBJDIR)/main.o $(OBJDIR)/menus.o $(OBJDIR)/comm.o $(OBJDIR)/ip.o \
	$(OBJDIR)/protocol.o $(OBJDIR)/serial.o $(OBJDIR)/language.o \
	$(OBJDIR)/print.o $(OBJDIR)/svg.o $(OBJDIR)/auto-update.o $(OBJDIR)/common.o

CFLAGS += -DAPPLICATION="\"judoweight\""
CFLAGS += -DAPP_NUM=4

### Windows executable ###
ifeq ($(TGT),WIN32OS)
OBJS += $(OBJDIR)/judoweight-icon.res

$(OBJDIR)/judoweight-icon.res: judoweight-icon.rc
	$(WINDRES) -O coff judoweight-icon.rc $(OBJDIR)/judoweight-icon.res

judoweight-icon.rc:
	echo "APPICONS ICON \"../etc/judoweight.ico\"" >judoweight-icon.rc

### Linux executable ###
else
endif

$(OBJDIR)/judoweight$(SUFF): $(OBJS)
	$(LD) -g -Wl,-rpath -Wl,bin $(OBJS) -o $(OBJDIR)/judoweight$(SUFF) $(LIBS)

$(OBJDIR)/avl.o: ../common/avl.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/ip.o: ../common/ip.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/protocol.o: ../common/protocol.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/binreloc.o: ../common/binreloc.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/serial.o: ../common/serial.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/language.o: ../common/language.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/common.o: ../common/common.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/auto-update.o: ../common/auto-update.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	${CC} -g $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/* *.res *.rc *~
