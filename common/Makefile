include ../common/Makefile.inc

MSGFMT=msgfmt
XGETTEXT=xgettext
MSGMERGE=msgmerge
pos := $(wildcard *.po)
mos := $(patsubst %.po,%.mo,$(pos))
csources := $(wildcard ../judotimer/*.c ../judoshiai/*.c ../judoinfo/*.c ../common/*.c ../judoweight/*.c ../judojudogi/*.c ../judoproxy/*.c)

locale: $(mos)

%.mo: %.po
	$(MSGFMT) -o $@ $< 

%.po: judoshiai.pot
	$(MSGMERGE) -U --backup=none $*.po judoshiai.pot

judoshiai.pot: $(csources)
	$(XGETTEXT) --from-code=UTF-8 -o judoshiai.pot --language=C --keyword=_ --keyword=N_ --add-location=file --package-name=judoshiai --package-version=$(SHIAI_VER_NUM) \
	$(csources)

.PHONY: clean merge locale

clean:
	rm -f $(mos)
	rm -f judoshiai.pot

merge: $(pos)
