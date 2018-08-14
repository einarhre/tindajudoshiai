include ../common/Makefile.inc

EXE = auto-update$(SUFF)
REM_INST = judoshiai-remote

OBJS = $(OBJDIR)/main.o $(OBJDIR)/sha1.o

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

ifeq ($(TGT),WIN32)
LIBS = -L$(WIN32DIR)/w32api/lib -lws2_32 -mwindows
else
LIBS =
endif

all: $(OBJDIR) $(OBJDIR)/$(EXE) $(OBJDIR)/REM_INST$(SUFF)

ifeq ($(TGT),WIN32)
OBJS += $(OBJDIR)/auto-update-icon.res

$(OBJDIR)/auto-update-icon.res: auto-update-icon.rc
	$(WINDRES) -O coff auto-update-icon.rc $(OBJDIR)/auto-update-icon.res

auto-update-icon.rc:
	echo "APPICONS ICON \"../etc/auto-update.ico\"" >auto-update-icon.rc

### Linux executable ###
else
endif

$(OBJDIR)/REM_INST$(SUFF): $(OBJDIR)/$(EXE)
ifeq ($(TGT),WIN32)
	sed "s/AppVerName=.*/AppVerName=Shiai $(SHIAI_VER_NUM)/" remote-inst.iss >ri1.iss
	sed "s/OutputBaseFilename=.*/OutputBaseFilename=$(REM_INST)-setup-$(SHIAI_VER_NUM)/" ri1.iss >ri2.iss
	sed "s,OBJDIR,$(OBJDIR)," ri2.iss | tr '/' '\\' >ri3.iss
	sed "s,RELDIR,$(RELEASEDIR)," ri3.iss | tr '/' '\\' >ri4.iss
	$(INNOSETUP) ri4.iss
	rm -f ri?.iss
else
	echo 'Not used in Linux' >$(OBJDIR)/REM_INST$(SUFF)
endif

$(OBJDIR)/$(EXE): $(OBJDIR) $(OBJS)
	$(LD) $(STATIC) $(OBJS) -o $(OBJDIR)/$(EXE) $(LIBS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c
	${CC} $(CFLAGS) -c $< -o $@

$(OBJDIR)/sha1.o: ../common/sha1.c
	${CC} $(PROF) $(CFLAGS) -c $< -o $@

install:
ifeq ($(TGT),WIN32)
	cp  $(OBJDIR)/$(REM_INST)*.exe $(RELDIR)/etc/bin/
endif

clean:
	rm -f $(OBJDIR)/*.o
