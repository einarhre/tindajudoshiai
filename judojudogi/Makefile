include ../mk/common.mk

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

all: $(OBJDIR) $(OBJDIR)/judojudogi$(SUFF)

$(OBJDIR):
	mkdir -p $(OBJDIR)

OBJS += $(OBJDIR)/main.o $(OBJDIR)/menus.o $(OBJDIR)/comm.o $(OBJDIR)/ip.o \
	$(OBJDIR)/protocol.o $(OBJDIR)/trees.o $(OBJDIR)/avl.o \
	$(OBJDIR)/language.o $(OBJDIR)/auto-update.o $(OBJDIR)/common.o \
	$(OBJDIR)/common-utils.o

CFLAGS += -DAPPLICATION="\"judojudogi\""
CFLAGS += -DAPP_NUM=5

### Windows executable ###
ifeq ($(TGT),WIN32OS)
OBJS += $(OBJDIR)/judojudogi-icon.res

$(OBJDIR)/judojudogi-icon.res: judojudogi-icon.rc
	$(WINDRES) -O coff judojudogi-icon.rc $(OBJDIR)/judojudogi-icon.res

judojudogi-icon.rc:
	echo "APPICONS ICON \"../etc/judojudogi.ico\"" >judojudogi-icon.rc

### Linux executable ###
else
endif

$(OBJDIR)/judojudogi$(SUFF): $(OBJS)
	$(LD) -g -Wl,-rpath -Wl,bin $(OBJS) -o $(OBJDIR)/judojudogi$(SUFF) $(LIBS)

$(OBJDIR)/avl.o: ../common/avl.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/ip.o: ../common/ip.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/protocol.o: ../common/protocol.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/binreloc.o: ../common/binreloc.c
	${CC} -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/language.o: ../common/language.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/common.o: ../common/common.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/auto-update.o: ../common/auto-update.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/common-utils.o: ../common/common-utils.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	${CC} -g $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/* *.res *.rc *~
