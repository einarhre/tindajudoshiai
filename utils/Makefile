include ../mk/common.mk

EXE = db-convert$(SUFF)

OBJS = $(OBJDIR)/main.o $(OBJDIR)/sqlite3.o

OBJDIR := $(JS_BUILD_DIR)/$(LASTPART)/$(OBJDIR)

ifeq ($(TGT),WIN32OS)
LIBS = -L$(WIN32DIR)/w32api/lib -lws2_32 -mwindows -mconsole
else
LIBS = -lpthread -ldl
endif

CFLAGS = -I../3rdparty/sqlite3

all: $(OBJDIR) $(OBJDIR)/$(EXE)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJDIR) $(OBJS)
	$(LD) -g -Wl,-rpath -Wl,bin $(OBJS) -o $(OBJDIR)/$(EXE) $(LIBS)

$(OBJDIR)/sqlite3.o: ../3rdparty/sqlite3/sqlite3.c
	${CC} $(PROF) -g $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	${CC} -g $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/* *.res *.rc *~

test:
	rm test-*
	echo
	$(OBJDIR)/$(EXE) -c test.shi
	echo
	$(OBJDIR)/$(EXE) test.shi
	echo
	$(OBJDIR)/$(EXE) -c test-29.shi
	echo
	sqlite3 test.shi .dump >test.txt
	sqlite3 test-0.shi .dump >test-0.txt
	sqlite3 test-29.shi .dump >test-29.txt
	sqlite3 test-29-0.shi .dump >test-29-0.txt
