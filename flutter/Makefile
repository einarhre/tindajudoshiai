include ../mk/common.mk

WEBDIR = $(JS_BUILD_DIR)/web
#FLAGS = --base-href=/web/weight/ --release --web-renderer html OR canvaskit
FLAGS = --web-renderer html
#--dart-define=FLUTTER_WEB_CANVASKIT_URL=/canvaskit/

all: $(WEBDIR) \
     $(WEBDIR)/shiai/index.html \
     $(WEBDIR)/weight/index.html \
     $(WEBDIR)/info/index.html \
     $(WEBDIR)/timer/index.html \
     $(WEBDIR)/referee/index.html \
     $(WEBDIR)/results/index.html

$(WEBDIR):
	mkdir -p $(WEBDIR)

$(WEBDIR)/shiai/index.html: $(wildcard judoshiai/lib/*.dart)
	cd judoshiai && $(FLUTTER) create --platforms web .
	cd judoshiai && $(FLUTTER) clean
	cd judoshiai && $(FLUTTER) pub get
	cd judoshiai && $(FLUTTER) build web --base-href=/web/shiai/ $(FLAGS)
	mkdir -p $(WEBDIR)/shiai
	cp -a judoshiai/build/web/* $(WEBDIR)/shiai/

judoweight/lib/database.g.dart: judoweight/lib/database.dart
	cd judoweight && $(FLUTTER) pub run build_runner build --delete-conflicting-outputs

$(WEBDIR)/weight/index.html: $(wildcard judoweight/lib/*.dart) judoweight/lib/database.g.dart
	cd judoweight && $(FLUTTER) create --platforms web .
	cd judoweight && $(FLUTTER) clean
	cd judoweight && $(FLUTTER) pub get
	cd judoweight && $(FLUTTER) build web --base-href=/web/weight/ $(FLAGS)
	mkdir -p $(WEBDIR)/weight
	cp -a judoweight/build/web/* $(WEBDIR)/weight/

$(WEBDIR)/info/index.html: $(wildcard judoinfo/lib/*.dart)
	cd judoinfo && $(FLUTTER) create --platforms web .
	cd judoinfo && $(FLUTTER) clean
	cd judoinfo && $(FLUTTER) pub get
	cd judoinfo && $(FLUTTER) build web --base-href=/web/info/ $(FLAGS)
	mkdir -p $(WEBDIR)/info
	cp -a judoinfo/build/web/* $(WEBDIR)/info/

$(WEBDIR)/timer/index.html: $(wildcard judotimer/lib/*.dart)
	cd judotimer && $(FLUTTER) create --platforms web .
	cd judotimer && $(FLUTTER) clean
	cd judotimer && $(FLUTTER) pub get
	cd judotimer && $(FLUTTER) build web --base-href=/web/timer/ $(FLAGS)
	mkdir -p $(WEBDIR)/timer
	cp -a judotimer/build/web/* $(WEBDIR)/timer/

$(WEBDIR)/referee/index.html: $(wildcard judoreferee/lib/*.dart)
	cd judoreferee && $(FLUTTER) create --platforms web .
	cd judoreferee && $(FLUTTER) clean
	cd judoreferee && $(FLUTTER) pub get
	cd judoreferee && $(FLUTTER) build web --base-href=/web/referee/ $(FLAGS)
	mkdir -p $(WEBDIR)/referee
	cp -a judoreferee/build/web/* $(WEBDIR)/referee/

$(WEBDIR)/results/index.html: $(wildcard results/lib/*.dart)
	cd results && $(FLUTTER) create --platforms web .
	cd results && $(FLUTTER) clean
	cd results && $(FLUTTER) pub get
	cd results && $(FLUTTER) build web --web-renderer html
	mkdir -p $(WEBDIR)/results
	cp -a results/build/web/* $(WEBDIR)/results/

clean:
	rm -r $(WEBDIR)
	cd judoshiai && $(FLUTTER) clean
	cd judotimer && $(FLUTTER) clean
	cd judoweight && $(FLUTTER) clean
	cd judoinfo && $(FLUTTER) clean
	cd results && $(FLUTTER) clean

test:
	cd results && $(FLUTTER) build web --web-renderer html
	mv results/build/web/index.html results/build/web/index1.html
	cp -a results/build/web/* $(HOME)/0tulokset/
