include ../mk/common.mk

ifeq ($(OS),Windows_NT)
DOCPATH=$(shell pwd -W | tr '/' '\\')\\
else
DOCPATH=$(shell pwd)/
endif

DOCSHIAI_FI=judoshiai-fi.pdf
DOCSHIAI_EN=judoshiai-en.pdf
DOCSHIAI_ES=judoshiai-es.pdf
DOCSHIAI_UK=judoshiai-uk.pdf
DOCSHIAI_NL=judoshiai-nl.pdf
DOCSHIAI_FR=judoshiai-fr.pdf
DOCTIMER_FI=judotimer-fi.pdf
DOCTIMER_EN=judotimer-en.pdf
DOCTIMER_ES=judotimer-es.pdf
DOCTIMER_SK=judotimer-sk.pdf
SQL_GUIDE_EN=sql-guide-en.pdf
LISP_EN=lisp_en.pdf
DOCFILES=$(DOCSHIAI_FI) $(DOCSHIAI_EN) $(DOCSHIAI_ES) $(DOCSHIAI_UK) $(DOCSHIAI_NL) $(DOCSHIAI_FR) \
 $(DOCTIMER_FI) $(DOCTIMER_EN) $(DOCTIMER_ES) $(SQL_GUIDE_EN) $(DOCTIMER_SK) \
 $(LISP_EN)

all: $(DOCFILES)

%.pdf: %.odt
	rm -f $@
	$(LIBREOFFICE) --headless --convert-to pdf $<

%.pdf: %.odg
	rm -f $@
	$(LIBREOFFICE) --headless --convert-to pdf $<

clean:
	rm -f *.pdf
