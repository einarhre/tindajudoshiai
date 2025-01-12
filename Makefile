include mk/common.mk

JUDOSHIAIFILE=$(JS_BUILD_DIR)/judoshiai/$(OBJDIR)/judoshiai$(SUFF)
JUDOTIMERFILE=$(JS_BUILD_DIR)/judotimer/$(OBJDIR)/judotimer$(SUFF)
JUDOINFOFILE=$(JS_BUILD_DIR)/judoinfo/$(OBJDIR)/judoinfo$(SUFF)
JUDOWEIGHTFILE=$(JS_BUILD_DIR)/judoweight/$(OBJDIR)/judoweight$(SUFF)
JUDOJUDOGIFILE=$(JS_BUILD_DIR)/judojudogi/$(OBJDIR)/judojudogi$(SUFF)
JUDOPROXYFILE=$(JS_BUILD_DIR)/judoproxy/$(OBJDIR)/judoproxy$(SUFF)
#AUTOUPDATEFILE=$(JS_BUILD_DIR)/auto-update/$(OBJDIR)/auto-update$(SUFF)
JUDOHTTPDFILE=$(JS_BUILD_DIR)/judohttpd/$(OBJDIR)/judohttpd$(SUFF)
DBCONVERTFILE=$(JS_BUILD_DIR)/utils/$(OBJDIR)/db-convert$(SUFF)

RELFILE=$(RELDIR)/bin/judoshiai$(SUFF)
RUNDIR=$(DEVELDIR)

ifeq ($(TOOL),MXE)
    #DLLS = $(wildcard $(MXE_BIN)/*.dll)

    DLLS = libao-4.dll libatk-1.0-0.dll libbz2.dll
    DLLS += libcairo-2.dll libcairo-gobject-2.dll libcroco-0.6-3.dll
    DLLS += libcurl-4.dll libepoxy-0.dll libexpat-1.dll
    DLLS += libffi-8.dll libfontconfig-1.dll libfreetype-6.dll
    DLLS += libgcrypt-20.dll libgdk-3-0.dll
    DLLS += libgdk_pixbuf-2.0-0.dll libgio-2.0-0.dll libglib-2.0-0.dll
    DLLS += libgmodule-2.0-0.dll
    DLLS += libgobject-2.0-0.dll libgthread-2.0-0.dll
    DLLS += libgtk-3-0.dll libharfbuzz-0.dll
    DLLS += libiconv-2.dll libidn2-0.dll libintl-8.dll
    DLLS += libjpeg-9.dll liblzma-5.dll libmpg123-0.dll
    DLLS += libpango-1.0-0.dll libpangocairo-1.0-0.dll
    DLLS += libpangowin32-1.0-0.dll
    DLLS += libpcre2-posix-3.dll libpcre2-8-0.dll libpcre2-16-0.dll
    DLLS += libpixman-1-0.dll libpng16-16.dll librsvg-2-2.dll
    DLLS += libssh2-1.dll libtiff-6.dll libunistring-5.dll
    DLLS += libwinpthread-1.dll libxml2-2.dll zlib1.dll libwebp-7.dll
    DLLS += libgpg-error-0.dll libgnutls-30.dll libgmp-10.dll
    DLLS += libhogweed-6.dll libnettle-8.dll libuv-1.dll
    DLLS += libfribidi-0.dll libtasn1-6.dll libwebsockets.dll

    ifeq ($(TARGETOS),WIN32)
        DLLS += libssl-3.dll libcrypto-3.dll libgcc_s_sjlj-1.dll
        DLLS += gspawn-win32-helper-console.exe
        DLLS += gspawn-win32-helper.exe
    else
        DLLS += libssl-3-x64.dll libcrypto-3-x64.dll libgcc_s_seh-1.dll
        DLLS += gspawn-win64-helper-console.exe
        DLLS += gspawn-win64-helper.exe
    endif
endif

ifeq ($(TOOL),MINGW)
    DLLS=$(shell ldd.exe /c/js-build/judoshiai/obj-win64/judoshiai.exe | grep mingw | awk '{print $3;}')
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
$(info Development directory:   $(DEVELDIR))
$(info Build directory:         $(JS_BUILD_DIR))
$(info Object directory:        $(OBJ_DIR))
$(info Release directory:       $(RELEASEDIR))
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
	mkdir -p $(RELDIR)/share/locale/da/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/he/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/fr/LC_MESSAGES
	mkdir -p $(RELDIR)/share/locale/fa/LC_MESSAGES
	mkdir -p $(RELDIR)/doc
	mkdir -p $(RELDIR)/licenses
	@echo "---------------------------"
	@echo "Run make in subdirectories"
	@echo "---------------------------"
	make -C flutter
	make -C common
	make -C judoshiai
	make -C judotimer
	make -C judoinfo
	make -C judoweight
	make -C judojudogi
	make -C serial
	#make -C auto-update
	make -C utils
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
	#cp $(AUTOUPDATEFILE) $(RELDIR)/bin/
	cp $(DBCONVERTFILE) $(RELDIR)/bin/
ifeq ($(JUDOHTTPD),YES)
	cp $(JUDOHTTPDFILE) $(RELDIR)/bin/
endif
ifeq ($(JUDOPROXY),YES)
	cp $(JUDOPROXYFILE) $(RELDIR)/bin/
endif

### Windows executable ###
ifeq ($(TGT),WIN32OS)
	mkdir -p $(RELDIR)/share/glib-2.0
	cp -r $(RUNDIR)/share/glib-2.0/schemas $(RELDIR)/share/glib-2.0/
	glib-compile-schemas $(RELDIR)/share/glib-2.0/schemas
	#make -C auto-update install
	@echo "---------------------------"
	@echo "Copy DLLs"
	@echo "---------------------------"
  ifeq ($(TOOL),MXE)
	#cp $(DLLS) $(RELDIR)/bin/
	cp $(foreach dll,$(DLLS),$(DEVELDIR)/bin/$(dll)) $(RELDIR)/bin/
	cp $(JS_BUILD_DIR)/judoshiai/$(OBJDIR)/microhttpd/src/microhttpd/.libs/libmicrohttpd-12.dll $(RELDIR)/bin/
  else # Not MXE, winxp or mingw
    ifeq ($(TARGETOS),WIN64) # mingw64
	cat mk/mingw-win64-dll.txt | while read LINE; do cp $$LINE $(RELDIR)/bin/; done
    else # WIN64
      ifeq ($(TARGETOS),WIN32) # mingw32
	cat mk/mingw-win32-dll.txt | while read LINE; do cp $$LINE $(RELDIR)/bin/; done
      else # WIN32 winxp
        ifeq ($(TOOL),MINGW)
	cat mk/mingw-winxp-dll.txt | while read LINE; do cp $$LINE $(RELDIR)/bin/; done
        else
	cp $(RUNDIR)/bin/*.dll $(RELDIR)/bin/
	cp $(SOUNDDIR)/bin/*.dll $(RELDIR)/bin/
	cp $(RSVGDIR)/bin/*.dll $(RELDIR)/bin/
	cp $(CURLDIR)/bin/*.dll $(RELDIR)/bin/
	cp $(SSH2DIR)/bin/*.dll $(RELDIR)/bin/
          ifeq ($(JUDOPROXY),YES)
	  cp $(WEBKITDIR)/bin/*.dll $(RELDIR)/bin/
	  cp $(SOAPDIR)/bin/*.dll $(RELDIR)/bin/
          endif # judoproxy
        endif # MINGW
	cp -r $(RUNDIR)/lib/gtk-$(GTKVER).0 $(RELDIR)/lib/
      endif # WIN32
    endif # WIN64
  endif # MXE

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
	cp -r $(RUNDIR)/share/locale/da $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/he $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/fr $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/fa $(RELDIR)/share/locale/
	cp -r $(RUNDIR)/share/locale/en_GB $(RELDIR)/share/locale/
	cp -r share/themes $(RELDIR)/share/
	cp -r share/icons $(RELDIR)/share/
	cp -r $(RUNDIR)/etc $(RELDIR)/
	cp -r $(JS_BUILD_DIR)/web $(RELDIR)/etc/

	mkdir -p $(RELDIR)/etc/gtk-3.0
	echo '[Settings]' >$(RELDIR)/etc/gtk-3.0/settings.ini
	echo '#gtk-theme-name=Adwaita' >>$(RELDIR)/etc/gtk-3.0/settings.ini
	echo '#gtk-theme-name=win32' >>$(RELDIR)/etc/gtk-3.0/settings.ini
endif # WIN32OS
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
	cp -r $(JS_BUILD_DIR)/web $(RELDIR)/etc/
	# Copy these manually before make. Distributed make doesn't provide the files automatically.
	#cp $(OBJDIR)/serial/$(OBJDIR)/websock-serial-pkg/websock-serial $(RELDIR)/etc/html/
	#cp $(JS_BUILD_DIR)/serial/obj-winxp/websock-serial-pkg.zip $(RELDIR)/etc/html/
	echo $(SHIAI_VER_NUM) >$(RELDIR)/etc/version.txt
	find $(RELDIR) | wc -l | tr -d "\r\n" >$(RELDIR)/filecount.txt
	@echo
	@echo "To make a setup executable run"
	@echo "  make setup"
	@echo
	@echo "To make a Debian package run (Linux only)"
	@echo "  make debian"

build_flutter:
	make -C flutter
	cp -r $(JS_BUILD_DIR)/web $(RELDIR)/etc/

setup:
ifeq ($(TGT),WIN32OS)
	sed "s/AppVerName=.*/AppVerName=Shiai $(SHIAI_VER_NUM)/" etc/judoshiai.iss >judoshiai1.iss
	sed "s/OutputBaseFilename=.*/OutputBaseFilename=judoshiai-setup-$(SHIAI_VER_NUM)-$(TGTEXT)/" judoshiai1.iss >judoshiai2.iss
	sed "s/TGTEXT/$(TGTEXT)/" judoshiai2.iss >judoshiai1.iss
ifeq ($(TGTEXT),64)
	sed "s/ARCHMODE/ArchitecturesInstallIn64BitMode=x64/" judoshiai1.iss >judoshiai2.iss
else
	sed "s/ARCHMODE//" judoshiai1.iss >judoshiai2.iss
endif
	sed "s,RELDIR,$(RELEASEDIR)," judoshiai2.iss | tr '/' '\\' >judoshiai1.iss
ifeq ($(TOOL),MINGW)
	echo "MINGW_DIR=$(MINGW_DIR)"
	sed "s,\(.home\),$(MINGW_DIR)\1," judoshiai1.iss >judoshiai2.iss
	cp judoshiai2.iss judoshiai1.iss
endif
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
ifeq ($(TGT),WIN32OS)
	sed "s/AppVerName=.*/AppVerName=Shiai $(SHIAI_VER_NUM)/" etc/remote-inst.iss >judoshiai1.iss
	sed "s,RELDIR,$(RELEASEDIR)," judoshiai1.iss | tr '/' '\\' >judoshiai2.iss
	$(INNOSETUP) judoshiai2.iss
	rm -f judoshiai*.iss
	mv $(RELEASEDIR)/remote-install.exe $(RELEASEDIR)/judoshiai/etc/
endif

install:
	cp -r $(RELDIR) /opt/
	ln -sf /opt/judoshiai/bin/judoshiai /usr/local/bin/judoshiai
	ln -sf /opt/judoshiai/bin/judotimer /usr/local/bin/judotimer
	ln -sf /opt/judoshiai/bin/judoinfo /usr/local/bin/judoinfo
	ln -sf /opt/judoshiai/bin/judoweight /usr/local/bin/judoweight
	ln -sf /opt/judoshiai/bin/judojudogi /usr/local/bin/judojudogi
	ln -sf /opt/judoshiai/bin/judoproxy /usr/local/bin/judoproxy
ifeq ($(JUDOHTTPD),YES)
	ln -sf /opt/judoshiai/bin/judohttpd /usr/local/bin/judohttpd
	cp gnome/judohttpd.desktop /usr/share/applications/
	cp etc/png/judohttpd.png /usr/share/pixmaps/
	cp etc/png/judohttpd.png /usr/share/icons/hicolor/48x48/apps/
endif
	desktop-file-install --rebuild-mime-info-cache --dir=/usr/local/share/applications gnome/judo*.desktop
	cp etc/png/judoshiai.png /usr/share/pixmaps/
	cp etc/png/judotimer.png /usr/share/pixmaps/
	cp etc/png/judoinfo.png /usr/share/pixmaps/
	cp etc/png/judoweight.png /usr/share/pixmaps/
	cp etc/png/judojudogi.png /usr/share/pixmaps/
	cp etc/png/judoproxy.png /usr/share/pixmaps/
	for pn in 16 24 32 48
	do
	  p="share/icons/hicolor/${pn}x${pn}/apps"
	  cp ${p}/judo*.png /usr/${p}/
	done
	gtk-update-icon-cache --force /usr/share/icons/hicolor
	#cp gnome/judoshiai.mime /usr/share/mime-info/
	#cp gnome/judoshiai.keys /usr/share/mime-info/
	#cp gnome/judoshiai.applications /usr/share/application-registry/
	#cp gnome/judoshiai.packages /usr/lib/mime/packages/judoshiai
	cp gnome/judoshiai.xml /usr/share/mime/packages/
	update-mime-database /usr/share/mime
	#cp gnome/judoshiai.menu /usr/share/menu/judoshiai

debian:
	rm -rf $(RELEASEDIR)/pkg
	mkdir -p $(RELEASEDIR)/pkg/usr/bin
	mkdir -p $(RELEASEDIR)/pkg/usr/lib
	mkdir -p $(RELEASEDIR)/pkg/usr/lib/mime/packages
	mkdir -p $(RELEASEDIR)/pkg/usr/share/applications
	mkdir -p $(RELEASEDIR)/pkg/usr/share/application-registry
	mkdir -p $(RELEASEDIR)/pkg/usr/share/menu
	mkdir -p $(RELEASEDIR)/pkg/usr/share/mime
	mkdir -p $(RELEASEDIR)/pkg/usr/share/mime-info
	mkdir -p $(RELEASEDIR)/pkg/usr/share/mime/packages
	mkdir -p $(RELEASEDIR)/pkg/usr/share/icons/hicolor/48x48/apps
	cp -a $(RELEASEDIR)/judoshiai $(RELEASEDIR)/pkg/usr/lib/
	ln -sf /usr/lib/judoshiai/programs/judoshiai $(RELEASEDIR)/pkg/usr/bin/judoshiai
	ln -sf /usr/lib/judoshiai/programs/judotimer $(RELEASEDIR)/pkg/usr/bin/judotimer
	ln -sf /usr/lib/judoshiai/programs/judoinfo $(RELEASEDIR)/pkg/usr/bin/judoinfo
	ln -sf /usr/lib/judoshiai/programs/judoweight $(RELEASEDIR)/pkg/usr/bin/judoweight
	ln -sf /usr/lib/judoshiai/programs/judojudogi $(RELEASEDIR)/pkg/usr/bin/judojudogi
	ln -sf /usr/lib/judoshiai/programs/judoproxy $(RELEASEDIR)/pkg/usr/bin/judoproxy
	cp gnome/judoshiai.desktop $(RELEASEDIR)/pkg/usr/share/applications/
	cp gnome/judotimer.desktop $(RELEASEDIR)/pkg/usr/share/applications/
	cp gnome/judoinfo.desktop $(RELEASEDIR)/pkg/usr/share/applications/
	cp gnome/judoweight.desktop $(RELEASEDIR)/pkg/usr/share/applications/
	cp gnome/judojudogi.desktop $(RELEASEDIR)/pkg/usr/share/applications/
	cp gnome/judoproxy.desktop $(RELEASEDIR)/pkg/usr/share/applications/
	cp etc/png/judoshiai.png $(RELEASEDIR)/pkg/usr/share/icons/hicolor/48x48/apps/
	cp etc/png/judotimer.png $(RELEASEDIR)/pkg/usr/share/icons/hicolor/48x48/apps/
	cp etc/png/judoinfo.png $(RELEASEDIR)/pkg/usr/share/icons/hicolor/48x48/apps/
	cp etc/png/judoweight.png $(RELEASEDIR)/pkg/usr/share/icons/hicolor/48x48/apps/
	cp etc/png/judojudogi.png $(RELEASEDIR)/pkg/usr/share/icons/hicolor/48x48/apps/
	cp etc/png/judoproxy.png $(RELEASEDIR)/pkg/usr/share/icons/hicolor/48x48/apps/
	cp gnome/judoshiai.mime $(RELEASEDIR)/pkg/usr/share/mime-info/
	cp gnome/judoshiai.keys $(RELEASEDIR)/pkg/usr/share/mime-info/
	cp gnome/judoshiai.applications $(RELEASEDIR)/pkg/usr/share/application-registry/
	cp gnome/judoshiai.packages $(RELEASEDIR)/pkg/usr/lib/mime/packages/judoshiai
	cp gnome/judoshiai.xml $(RELEASEDIR)/pkg/usr/share/mime/packages/
	cp gnome/judoshiai.menu $(RELEASEDIR)/pkg/usr/share/menu/judoshiai
	mv $(RELEASEDIR)/pkg/usr/lib/judoshiai/bin $(RELEASEDIR)/pkg/usr/lib/judoshiai/programs
	fpm $(ARCHITECTURE) -s dir -t deb -C $(RELEASEDIR)/pkg --name judoshiai --version $(SHIAI_VER_NUM) --iteration 1 \
	-d libao4 -d libatk1.0-0 -d libcairo2 -d libcurl4 -d libgdk-pixbuf2.0-0 -d libgtk-3-0 \
	-d libpango-1.0-0 -d librsvg2-2 -d libssh2-1 -d libuv1 -d libgnutls30 \
	--description "JudoShiai Package" --deb-no-default-config-files
	mv *.deb $(RELEASEDIR)/

appimage:
	mk/make-appimage.sh $(RELEASEDIR) $(TARGETOS) $(SHIAI_VER_NUM) judoshiai
	mk/make-appimage.sh $(RELEASEDIR) $(TARGETOS) $(SHIAI_VER_NUM) judotimer
	mk/make-appimage.sh $(RELEASEDIR) $(TARGETOS) $(SHIAI_VER_NUM) judoinfo
	mk/make-appimage.sh $(RELEASEDIR) $(TARGETOS) $(SHIAI_VER_NUM) judoweight
	mk/make-appimage.sh $(RELEASEDIR) $(TARGETOS) $(SHIAI_VER_NUM) judojudogi

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

