include common/Makefile.inc

JUDOSHIAIFILE=$(JS_BUILD_DIR)/judoshiai/$(OBJDIR)/judoshiai$(SUFF)
JUDOTIMERFILE=$(JS_BUILD_DIR)/judotimer/$(OBJDIR)/judotimer$(SUFF)
JUDOINFOFILE=$(JS_BUILD_DIR)/judoinfo/$(OBJDIR)/judoinfo$(SUFF)
JUDOWEIGHTFILE=$(JS_BUILD_DIR)/judoweight/$(OBJDIR)/judoweight$(SUFF)
JUDOJUDOGIFILE=$(JS_BUILD_DIR)/judojudogi/$(OBJDIR)/judojudogi$(SUFF)
JUDOPROXYFILE=$(JS_BUILD_DIR)/judoproxy/$(OBJDIR)/judoproxy$(SUFF)
AUTOUPDATEFILE=$(JS_BUILD_DIR)/auto-update/$(OBJDIR)/auto-update$(SUFF)
JUDOHTTPDFILE=$(JS_BUILD_DIR)/judohttpd/$(OBJDIR)/judohttpd$(SUFF)

RELFILE=$(RELDIR)/bin/judoshiai$(SUFF)
RUNDIR=$(DEVELDIR)

ifeq ($(TOOL),MXE)
    DLLS = libao-4.dll libatk-1.0-0.dll libbz2.dll
    DLLS += libcairo-2.dll libcairo-gobject-2.dll libcroco-0.6-3.dll
    DLLS += libcurl-4.dll libepoxy-0.dll libexpat-1.dll
    DLLS += libffi-6.dll libfontconfig-1.dll libfreetype-6.dll
    DLLS += libgcrypt-20.dll libgdk-3-0.dll
    DLLS += libgdk_pixbuf-2.0-0.dll libgio-2.0-0.dll libglib-2.0-0.dll
    DLLS += libgmodule-2.0-0.dll libgmp-10.dll libgnutls-30.dll
    DLLS += libgobject-2.0-0.dll libgthread-2.0-0.dll
    DLLS += libgtk-3-0.dll libharfbuzz-0.dll libhogweed-4.dll libhogweed-5.dll
    DLLS += libiconv-2.dll libidn2-0.dll libintl-8.dll
    DLLS += libjpeg-9.dll liblzma-5.dll libmpg123-0.dll
    DLLS += libnettle-6.dll libnettle-7.dll libpango-1.0-0.dll libpangocairo-1.0-0.dll
    DLLS += libpangoft2-1.0-0.dll libpangowin32-1.0-0.dll libpcre-1.dll
    DLLS += libpixman-1-0.dll libpng16-16.dll librsvg-2-2.dll
    DLLS += libssh2-1.dll libtiff-5.dll libunistring-2.dll
    DLLS += libwinpthread-1.dll libxml2-2.dll zlib1.dll libwebp-7.dll
    DLLS += libgpg-error-0.dll libtasn1-6.dll

    ifeq ($(TARGETOS),WIN32)
        DLLS += libgcc_s_sjlj-1.dll libcrypto-1_1.dll
    else
        DLLS += libgpg-error6-0.dll libgcc_s_seh-1.dll libcrypto-1_1-x64.dll
    endif
endif

$(info --- MAKE VARIABLES: ---)
$(info User:                    $(USER))
$(info Environment:             $(OS))
$(info Target operating system: $(TARGETOS))
$(info Suffix text:             $(TGTEXT))
$(info Target architecture:     $(TGT))
$(info Work directory:          $(CURDIR))
$(info Build JudoHttpd:         $(JUDOHTTPD))
$(info Build JudoProxy:         $(JUDOPROXY))
$(info Used tool for Windowns:  $(TOOL))
$(info MXE directory:           $(MXEDIR))
$(info WIN32 base directory:    $(WIN32_BASE))
$(info -----------------------)

all:
	@echo "---------------------------"
	@echo "Create release directories"
	@echo "---------------------------"
	rm -rf $(RELDIR)
	mkdir -p $(RELDIR)/bin
	mkdir -p $(RELDIR)/share/locale/
	mkdir -p $(RELDIR)/share/locale/fi/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/sv/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/es/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/et/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/uk/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/is/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/nb/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/pl/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/sk/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/nl/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/cs/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/de/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/ru/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/da/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/he/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/fr/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/fa/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/en_GB/LC_MESSAGES
	mkdir -p $(RELDIR)/share/themes
	mkdir -p $(RELDIR)/share/icons
	mkdir -p $(RELDIR)/lib
	mkdir -p $(RELDIR)/doc
	mkdir -p $(RELDIR)/licenses
	mkdir -p $(RELDIR)/etc/www/js
	mkdir -p $(RELDIR)/etc/www/css
	mkdir -p $(RELDIR)/etc/bin
	@echo "---------------------------"
	@echo "Run make in subdirectories"
	@echo "---------------------------"
	make -C common
	make -C judoshiai
	make -C judotimer
	make -C judoinfo
	make -C judoweight
	make -C judojudogi
	make -C serial
	make -C auto-update
ifeq ($(JUDOHTTPD),YES)
	make -C judohttpd
endif
ifeq ($(JUDOPROXY),YES)
	make -C judoproxy
endif
	make -C doc
	@echo "---------------------------"
	@echo "Copy program files"
	@echo "---------------------------"
	cp $(JUDOSHIAIFILE) $(RELDIR)/bin/
	cp $(JUDOTIMERFILE) $(RELDIR)/bin/
	cp $(JUDOINFOFILE) $(RELDIR)/bin/
	cp $(JUDOWEIGHTFILE) $(RELDIR)/bin/
	cp $(JUDOJUDOGIFILE) $(RELDIR)/bin/
	cp $(AUTOUPDATEFILE) $(RELDIR)/bin/
ifeq ($(JUDOHTTPD),YES)
	cp $(JUDOHTTPDFILE) $(RELDIR)/bin/
endif
ifeq ($(JUDOPROXY),YES)
	cp $(JUDOPROXYFILE) $(RELDIR)/bin/
endif

### Windows executable ###
ifeq ($(TGT),WIN32)
ifeq ($(GTKVER),3)
	mkdir -p $(RELDIR)/share/glib-2.0
	cp -r $(RUNDIR)/share/glib-2.0/schemas $(RELDIR)/share/glib-2.0/
endif
	make -C auto-update install
	@echo "---------------------------"
	@echo "Copy DLLs"
	@echo "---------------------------"
ifeq ($(TOOL),MXE)
	    cp $(foreach dll,$(DLLS),$(DEVELDIR)/bin/$(dll)) $(RELDIR)/bin/
else
	    cp $(RUNDIR)/bin/*.dll $(RELDIR)/bin/
	    cp $(SOUNDDIR)/bin/*.dll $(RELDIR)/bin/
	    cp $(RSVGDIR)/bin/*.dll $(RELDIR)/bin/
	    cp $(CURLDIR)/bin/*.dll $(RELDIR)/bin/
	    cp $(SSH2DIR)/bin/*.dll $(RELDIR)/bin/
            ifeq ($(JUDOPROXY),YES)
		cp $(WEBKITDIR)/bin/*.dll $(RELDIR)/bin/
		cp $(SOAPDIR)/bin/*.dll $(RELDIR)/bin/
            endif
	cp -r $(RUNDIR)/lib/gtk-$(GTKVER).0 $(RELDIR)/lib/
endif
	@echo "---------------------------"
	@echo "Copy share files"
	@echo "---------------------------"
	cp -r $(RUNDIR)/share/locale/fi $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/sv $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/es $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/et $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/uk $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/is $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/nb $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/pl $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/sk $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/nl $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/cs $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/de $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/ru $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/da $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/he $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/fr $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/fa $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/en_GB $(RELDIR)/share/locale/
	cp -r share/themes $(RELDIR)/share/
	cp -r share/icons $(RELDIR)/share/
	cp -r $(RUNDIR)/etc $(RELDIR)/

	mkdir -p $(RELDIR)/etc/gtk-3.0
	echo '[Settings]' >$(RELDIR)/etc/gtk-3.0/settings.ini
	echo '#gtk-theme-name=Adwaita' >>$(RELDIR)/etc/gtk-3.0/settings.ini
	echo '#gtk-theme-name=win32' >>$(RELDIR)/etc/gtk-3.0/settings.ini
endif
	@echo "---------------------------"
	@echo "Copy documents"
	@echo "---------------------------"
	cp doc/*.pdf $(RELDIR)/doc/
	@echo "---------------------------"
	@echo "Copy translations"
	@echo "---------------------------"
	cp common/judoshiai-fi_FI.mo $(RELDIR)/share/locale/fi/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-sv_SE.mo $(RELDIR)/share/locale/sv/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-es_ES.mo $(RELDIR)/share/locale/es/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-et_EE.mo $(RELDIR)/share/locale/et/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-uk_UA.mo $(RELDIR)/share/locale/uk/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-is_IS.mo $(RELDIR)/share/locale/is/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-nb_NO.mo $(RELDIR)/share/locale/nb/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-pl_PL.mo $(RELDIR)/share/locale/pl/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-sk_SK.mo $(RELDIR)/share/locale/sk/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-nl_NL.mo $(RELDIR)/share/locale/nl/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-cs_CZ.mo $(RELDIR)/share/locale/cs/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-de_DE.mo $(RELDIR)/share/locale/de/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-ru_RU.mo $(RELDIR)/share/locale/ru/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-da_DK.mo $(RELDIR)/share/locale/da/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-he_IL.mo $(RELDIR)/share/locale/he/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-fr_FR.mo $(RELDIR)/share/locale/fr/LC_MESSAGES/judoshiai.mo
	cp common/judoshiai-fa_IR.mo $(RELDIR)/share/locale/fa/LC_MESSAGES/judoshiai.mo
	@echo "---------------------------"
	@echo "Copy other files"
	@echo "---------------------------"
	cp -r etc $(RELDIR)/
	cp licenses/* $(RELDIR)/licenses
	cp -r svg $(RELDIR)/
	cp -r custom-examples $(RELDIR)/
	cp -r svg-lisp $(RELDIR)/
	-cp $(JS_BUILD_DIR)/serial/obj-linux/websock-serial-pkg/websock-serial $(RELDIR)/etc/html/
	-cp $(JS_BUILD_DIR)/serial/obj-winxp/websock-serial-pkg.zip $(RELDIR)/etc/html/
	echo $(SHIAI_VER_NUM) >$(RELDIR)/etc/version.txt
	find $(RELDIR) | wc -l | tr -d "\r\n" >$(RELDIR)/filecount.txt
	@echo
	@echo "To make a setup executable run"
	@echo "  make setup"
	@echo
	@echo "To make a Debian package run (Linux only)"
	@echo "  sudo -E JS_BUILD_DIR=$(JS_BUILD_DIR) make debian"

setup:
ifeq ($(TGT),WIN32)
	sed "s/AppVerName=.*/AppVerName=Shiai $(SHIAI_VER_NUM)/" etc/judoshiai.iss >judoshiai1.iss
	sed "s/OutputBaseFilename=.*/OutputBaseFilename=judoshiai-setup-$(SHIAI_VER_NUM)-$(TGTEXT)/" judoshiai1.iss >judoshiai2.iss
	sed "s/TGTEXT/$(TGTEXT)/" judoshiai2.iss >judoshiai1.iss
ifeq ($(TGTEXT),64)
	sed "s/ARCHMODE/ArchitecturesInstallIn64BitMode=x64/" judoshiai1.iss >judoshiai2.iss
else
	sed "s/ARCHMODE//" judoshiai1.iss >judoshiai2.iss
endif
	sed "s,RELDIR,$(RELEASEDIR)," judoshiai2.iss | tr '/' '\\' >judoshiai1.iss
	$(INNOSETUP) judoshiai1.iss
	rm -f judoshiai*.iss
else
	tar -C $(RELEASEDIR) -cf judoshiai.tar judoshiai
	tar -C etc -rf judoshiai.tar install.sh
	gzip judoshiai.tar
	cat etc/header.sh judoshiai.tar.gz >$(RELEASEDIR)/judoshiai-setup-$(SHIAI_VER_NUM).bin
	chmod a+x $(RELEASEDIR)/judoshiai-setup-$(SHIAI_VER_NUM).bin
	rm -f judoshiai.tar.gz
endif

$(RELEASEDIR)/judoshiai/etc/remote-install.exe:
ifeq ($(TGT),WIN32)
	sed "s/AppVerName=.*/AppVerName=Shiai $(SHIAI_VER_NUM)/" etc/remote-inst.iss >judoshiai1.iss
	sed "s,RELDIR,$(RELEASEDIR)," judoshiai1.iss | tr '/' '\\' >judoshiai2.iss
	$(INNOSETUP) judoshiai2.iss
	rm -f judoshiai*.iss
	mv $(RELEASEDIR)/remote-install.exe $(RELEASEDIR)/judoshiai/etc/
endif

install:
	cp -r $(RELDIR) /usr/lib/
	ln -sf /usr/lib/judoshiai/bin/judoshiai /usr/bin/judoshiai
	ln -sf /usr/lib/judoshiai/bin/judotimer /usr/bin/judotimer
	ln -sf /usr/lib/judoshiai/bin/judoinfo /usr/bin/judoinfo
	ln -sf /usr/lib/judoshiai/bin/judoweight /usr/bin/judoweight
	ln -sf /usr/lib/judoshiai/bin/judojudogi /usr/bin/judojudogi
	ln -sf /usr/lib/judoshiai/bin/judoproxy /usr/bin/judoproxy
ifeq ($(JUDOHTTPD),YES)
	ln -sf /usr/lib/judoshiai/bin/judohttpd /usr/bin/judohttpd
	cp gnome/judohttpd.desktop /usr/share/applications/
	cp etc/png/judohttpd.png /usr/share/pixmaps/
endif
	cp gnome/judoshiai.desktop /usr/share/applications/
	cp gnome/judotimer.desktop /usr/share/applications/
	cp gnome/judoinfo.desktop /usr/share/applications/
	cp gnome/judoweight.desktop /usr/share/applications/
	cp gnome/judojudogi.desktop /usr/share/applications/
	cp gnome/judoproxy.desktop /usr/share/applications/
	cp etc/png/judoshiai.png /usr/share/pixmaps/
	cp etc/png/judotimer.png /usr/share/pixmaps/
	cp etc/png/judoinfo.png /usr/share/pixmaps/
	cp etc/png/judoweight.png /usr/share/pixmaps/
	cp etc/png/judojudogi.png /usr/share/pixmaps/
	cp etc/png/judoproxy.png /usr/share/pixmaps/
	cp gnome/judoshiai.mime /usr/share/mime-info/
	cp gnome/judoshiai.keys /usr/share/mime-info/
	cp gnome/judoshiai.applications /usr/share/application-registry/
	cp gnome/judoshiai.packages /usr/lib/mime/packages/judoshiai
	cp gnome/judoshiai.xml /usr/share/mime/packages/
	cp gnome/judoshiai.menu /usr/share/menu/judoshiai

debian:
	cp gnome/*-pak .
	checkinstall -y -D --install=no --pkgname=judoshiai --pkgversion=$(SHIAI_VER_NUM) \
	--maintainer=oh2ncp@kolumbus.fi --nodoc \
	--requires libao4,libatk1.0-0,libcairo2,libcurl3,libgdk-pixbuf2.0-0,libgtk-3-0,libpango-1.0-0,librsvg2-2
	chown $(USER):$(USER) *.deb
	mv *.deb $(RELDIR)/
	rm description-pak postinstall-pak postremove-pak

clean:
	make -C common clean
	make -C judoshiai clean
	make -C judotimer clean
	make -C judoinfo clean
	make -C judoweight clean
	make -C judojudogi clean
	make -C judoproxy clean
ifeq ($(JUDOHTTPD),YES)
	make -C judohttpd clean
endif
	rm -rf $(RELEASEDIR)

