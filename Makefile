DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))

NODE_MODS  := $(DIR)/node_modules
JADE       := $(NODE_MODS)/jade/bin/jade.js
STYLUS     := $(NODE_MODS)/stylus/bin/stylus
AP         := $(NODE_MODS)/autoprefixer/autoprefixer
BROWSERIFY := $(NODE_MODS)/browserify/bin/cmd.js

HTTP      := http
HTML      := index
HTML      := $(patsubst %,$(HTTP)/%.html,$(HTML))
CSS       := style
CSS       := $(patsubst %,$(HTTP)/%.css,$(CSS))
JS        := $(wildcard src/js/*.js)
JS_ASSETS := $(HTTP)/js/assets.js
TEMPLS    := $(wildcard src/jade/templates/*.jade)
STATIC    := $(shell find src/resources -type f)
STATIC    := $(patsubst src/resources/%,http/%,$(STATIC))

ifndef DEST
DEST=bbctrl/
endif

WATCH := src/jade src/stylus src/js Makefile

TARGETS := $(HTML) $(CSS) $(JS_ASSETS) $(STATIC)

all: node_modules $(TARGETS)

copy: $(TARGETS)
	cp -r *.py inevent http/ $(DEST)

$(HTTP)/admin.html: build/templates.jade

$(HTTP)/%.html: src/jade/%.jade
	$(JADE) -P $< --out $(shell dirname $@) || \
	(rm -f $@; exit 1)

$(HTTP)/%.css: src/stylus/%.styl
	mkdir -p $(shell dirname $@)
	$(STYLUS) < $< > $@ || (rm -f $@; exit 1)

$(HTTP)/%: src/resources/%
	install -D $< $@

build/templates.jade: $(TEMPLS)
	mkdir -p build
	cat $(TEMPLS) >$@

$(JS_ASSETS): $(JS)
	@mkdir -p $(shell dirname $@)
	$(BROWSERIFY) src/js/main.js -s main -o $@ || \
	(rm -f $@; exit 1)

node_modules:
	npm install

watch:
	@clear
	$(MAKE)
	@while sleep 1; do \
	  inotifywait -qr -e modify -e create -e delete \
		--exclude .*~ --exclude \#.* $(WATCH); \
	  clear; \
	  $(MAKE); \
	done

clean:
	rm -rf $(HTTP)
