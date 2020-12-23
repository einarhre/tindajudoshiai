include ../mk/common.mk

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

all: $(OBJDIR) $(OBJDIR)/judotimer$(SUFF)

$(OBJDIR):
	mkdir -p $(OBJDIR)

OBJS += $(OBJDIR)/main.o $(OBJDIR)/menus.o $(OBJDIR)/stopwatch.o $(OBJDIR)/comm.o \
	$(OBJDIR)/ip.o $(OBJDIR)/protocol.o $(OBJDIR)/show-ad.o $(OBJDIR)/sound.o \
	$(OBJDIR)/video.o $(OBJDIR)/language.o $(OBJDIR)/common.o $(OBJDIR)/auto-update.o \
	 $(OBJDIR)/common-utils.o

CFLAGS += -DAPPLICATION="\"judotimer\""
CFLAGS += -DAPP_NUM=2

### WindowsXP executable ###
ifeq ($(TGT),WIN32OS)
  CFLAGS += -I$(SOUNDDIR)/include
  ifeq ($(TOOL),MXE)
    LIBS += -lmpg123 -lao
  else # MXE
    ifeq ($(TOOL),MINGW)
      LIBS += $(shell $(PKGCONFIGPATH) $(PKGCONFIG) --libs libmpg123) -lao
    else # MINGW
      LIBS += -L$(SOUNDDIR)/bin -lmpg123-0 -lao-4
    endif # MINGW
  endif # MXE

  RPATH =
  OBJS += $(OBJDIR)/judotimer-icon.res

$(OBJDIR)/judotimer-icon.res: judotimer-icon.rc
	$(WINDRES) -O coff judotimer-icon.rc $(OBJDIR)/judotimer-icon.res

judotimer-icon.rc:
	echo "APPICONS ICON \"../etc/judotimer.ico\"" >judotimer-icon.rc

### Linux executable ###
else # WIN32
  LIBS += -lmpg123 -lao
  RPATH = -Wl,-rpath,/usr/lib/judoshiai/lib
endif # WIN32

$(OBJDIR)/judotimer$(SUFF): $(OBJS)
	${LD} -g $(RPATH) $(OBJS) -o $(OBJDIR)/judotimer$(SUFF) $(LIBS)

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

$(OBJDIR)/%.o: %.c judotimer.h ../common/comm.h
	${CC} -g $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/* *.res *.rc *~
