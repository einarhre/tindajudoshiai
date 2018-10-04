include ../common/Makefile.inc

MSGFMT=msgfmt
XGETTEXT=xgettext
MSGMERGE=msgmerge
pos=$(wildcard *.po)
mos=$(patsubst %.po,%.mo,$(pos))

#This sorted list of all csources ensures, that xgettext will allways produce 
#exactly the same order of all msgid, thus making changes in the msgids easily 
#trackable via git. Alternatively one could pass the --sort-by-file option to
#xgettext, but this gives you more control over the desired order.
csources=$(sort $(wildcard ../judotimer/*.c))
csources+=$(sort $(wildcard ../judoshiai/*.c))
csources+=$(sort $(wildcard ../judoinfo/*.c))
csources+=$(sort $(wildcard ../common/*.c))
csources+=$(sort $(wildcard ../judoweight/*.c))
csources+=$(sort $(wildcard ../judojudogi/*.c))
csources+=$(sort $(wildcard ../judoproxy/*.c))

locale: MSGMERGE_EXTRAOPTIONS=--no-fuzzy-matching
locale: $(mos)

%.mo: %.po
	$(MSGFMT) -o $@ $< 

%.po: judoshiai.pot
	$(MSGMERGE) -U $(MSGMERGE_EXTRAOPTIONS) --backup=none $*.po judoshiai.pot

#produces the po template file wich will be the starting point for any new 
#translation and is used to update existing po files.
judoshiai.pot: $(csources)
    #The --add-location=file option ensures that the context of a msgid is 
    #visible to translators, but the line numbers are omited thus reducing the 
    #amount of changes seen on po files.
	$(XGETTEXT) --from-code=UTF-8 --output=judoshiai.pot --language=C --keyword=_ --keyword=N_ --add-location=file \
	--package-name=judoshiai --package-version=$(SHIAI_VER_NUM) --copyright-holder='Hannu Jokinen' --msgid-bugs-address=oh2ncp@kolumbus.fi \
	$(csources)
	#POT-Creation-Date will allways be updatet by msgmerge, we do not want to 
	#have changed po files on every build, thus we delete this line in the po 
	#template.
	ifdef MSGMERGE_EXTRAOPTIONS
	sed -i '/POT-Creation-Date/d' judoshiai.pot
	endif

.PHONY: clean merge locale

clean:
	rm -f $(mos)
	rm -f judoshiai.pot

#Translators should call this target once before they want to update theire 
#Language file. It will update all msgids and try fuzzy matching. The
#translator should then remove all fuzzy markers after having reviewed them.
merge: $(pos)
