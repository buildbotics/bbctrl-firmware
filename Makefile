DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))

NODE_MODS  := $(DIR)/node_modules
JADE       := $(NODE_MODS)/jade/bin/jade.js
STYLUS     := $(NODE_MODS)/stylus/bin/stylus
AP         := $(NODE_MODS)/autoprefixer/autoprefixer
BROWSERIFY := $(NODE_MODS)/browserify/bin/cmd.js

HTML      := index
HTML      := $(patsubst %,http/%.html,$(HTML))
CSS       := $(wildcard src/stylus/*.styl)
CSS_ASSETS := build/css/style.css
JS        := $(wildcard src/js/*.js)
JS_ASSETS := http/js/assets.js
STATIC    := $(shell find src/resources -type f)
STATIC    := $(patsubst src/resources/%,http/%,$(STATIC))
TEMPLS    := $(wildcard src/jade/templates/*.jade)

ifndef DEST
DEST=bbctrl/
endif

WATCH := src/jade src/jade/templates src/stylus src/js src/resources Makefile

all: html css js static

copy: all
	cp -r *.py inevent http/ $(DEST)

mount:
	mkdir -p $(DEST)
	sshfs bbmc@bbctrl.local: $(DEST)

umount:
	fusermount -u $(DEST)

html: templates $(HTML)

css: $(CSS_ASSETS) $(CSS_ASSETS).sha256
	install -D $< http/css/style-$(shell cat $(CSS_ASSETS).sha256).css

js: $(JS_ASSETS) $(JS_ASSETS).sha256
	install -D $< http/js/assets-$(shell cat $(JS_ASSETS).sha256).js

static: $(STATIC)

templates: build/templates.jade

build/templates.jade: $(TEMPLS)
	mkdir -p build
	cat $(TEMPLS) >$@

build/hashes.jade: $(CSS_ASSETS).sha256 $(JS_ASSETS).sha256
	echo "- var css_hash = '$(shell cat $(CSS_ASSETS).sha256)'" > $@
	echo "- var js_hash = '$(shell cat $(JS_ASSETS).sha256)'" >> $@

http/index.html: build/templates.jade build/hashes.jade

$(JS_ASSETS): $(JS) node_modules
	@mkdir -p $(shell dirname $@)
	$(BROWSERIFY) src/js/main.js -s main -o $@ || \
	(rm -f $@; exit 1)

node_modules:
	npm install

%.sha256: %
	mkdir -p $(shell dirname $@)
	sha256sum $< | sed 's/^\([a-f0-9]\+\) .*$$/\1/' > $@

http/%: src/resources/%
	install -D $< $@

http/%.html: src/jade/%.jade $(wildcard src/jade/*.jade) node_modules
	@mkdir -p $(shell dirname $@)
	$(JADE) -P $< -o http || (rm -f $@; exit 1)

build/css/%.css: src/stylus/%.styl node_modules
	mkdir -p $(shell dirname $@)
	$(STYLUS) < $< > $@ || (rm -f $@; exit 1)

watch:
	@clear
	$(MAKE)
	@while sleep 1; do \
	  inotifywait -qr -e modify -e create -e delete \
		--exclude .*~ --exclude \#.* $(WATCH); \
	  clear; \
	  $(MAKE); \
	done

tidy:
	rm -f $(shell find "$(DIR)" -name \*~)

clean: tidy
	rm -rf build html

dist-clean: clean
	rm -rf node_modules

.PHONY: all install html css static templates clean tidy copy mount umount
