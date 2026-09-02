DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))

NODE_MODS  := $(DIR)/node_modules
PUG        := $(NODE_MODS)/.bin/pug
STYLUS     := $(NODE_MODS)/.bin/stylus

TARGET_DIR := build/http
HTML       := index
HTML       := $(patsubst %,$(TARGET_DIR)/%.html,$(HTML))
RESOURCES  := $(shell find src/resources -type f)
RESOURCES  := $(patsubst src/resources/%,$(TARGET_DIR)/%,$(RESOURCES))
TEMPLS     := $(wildcard src/pug/templates/*.pug)

SHARE           := share
DEV_ROOT        := build/dev
ARM_BINS        := bin/camotics.so bin/bbkbd bin/updiprog bin/rpipdi

RSYNC_EXCLUDE := \*.pyc __pycache__ \*.egg-info \\\#* \*~ .\\\#\*
RSYNC_EXCLUDE := $(patsubst %,--exclude %,$(RSYNC_EXCLUDE))
RSYNC_OPTS    := $(RSYNC_EXCLUDE) -rv --no-g --delete --force

VERSION  := $(shell sed -n 's/^.*"version": "\([^"]*\)",.*$$/\1/p' package.json)
PKG_NAME := bbctrl-$(VERSION)
PUB_PATH := root@buildbotics.com:/var/www/buildbotics.com/bbctrl-2.0
BETA_VERSION := $(VERSION)-rc$(shell ./scripts/next-rc)
BETA_PKG_NAME := bbctrl-$(BETA_VERSION)
CUR_BETA_VERSION := $(shell cat dist/latest-beta.txt)
CUR_BETA_PKG_NAME := bbctrl-$(CUR_BETA_VERSION)

IMAGE := $(shell date +%Y%m%d)-debian-bookworm-bbctrl.img
IMG_PATH := root@buildbotics.com:/var/www/buildbotics.com/upload

CPUS := $(shell grep -c ^processor /proc/cpuinfo)

SUBPROJECTS := avr pwr
SUBPROJECTS := $(patsubst %,src/%,$(SUBPROJECTS))
$(info SUBPROJECTS="$(SUBPROJECTS)")

WATCH := src/pug src/pug/templates src/stylus src/js src/resources Makefile
WATCH += src/static

ifndef HOST
HOST=bbctrl.local
endif


all: html $(SUBPROJECTS)

.PHONY: $(SUBPROJECTS)
$(SUBPROJECTS):
	$(MAKE) -C $@

html: resources $(HTML)
resources: $(RESOURCES)

demo: html resources bbemu
	ln -sf ../../../$(TARGET_DIR) src/py/bbctrl/http
	./setup.py install
	cp src/avr/emu/bbemu /usr/local/bin

bbemu:
	$(MAKE) -C src/avr/emu

pkg: all $(SUBPROJECTS)
	#cp -a $(SHARE)/camotics/tpl_lib src/py/bbctrl/
	./setup.py sdist

beta-pkg: pkg
	cp dist/$(PKG_NAME).tar.bz2 dist/$(BETA_PKG_NAME).tar.bz2
	echo -n $(BETA_VERSION) > dist/latest-beta.txt

arm-bin: $(ARM_BINS)
camotics: bin/camotics.so

# Emulated aarch64 build environment.  Order-only prerequisite below so an
# existing chroot is never rebuilt on a timestamp change.
$(DEV_ROOT):
	sudo debootstrap --arch=arm64 --foreign \
	  --components main,contrib,non-free-firmware bookworm $@
	sudo cp -av /usr/bin/qemu-aarch64-static $@/usr/bin/
	sudo ./scripts/chroot-run $@ /debootstrap/debootstrap --second-stage

# init-devfs.sh builds every ARM project in one pass, so the copy out of the
# chroot is per-binary but the build is not.
.PHONY: dev-build
dev-build: | $(DEV_ROOT)
	sudo install -m 755 scripts/init-devfs.sh $(DEV_ROOT)/root/
	sudo ./scripts/chroot-run $(DEV_ROOT) /bin/bash /root/init-devfs.sh

bin/camotics.so: DEV_BIN := camotics/build/camotics.so
bin/bbkbd:       DEV_BIN := bbkbd/bbkbd
bin/updiprog:    DEV_BIN := updiprog/updiprog
bin/rpipdi:      DEV_BIN := rpipdi/rpipdi

$(ARM_BINS): dev-build
	install -m 755 -D $(DEV_ROOT)/$(DEV_BIN) $@

%.img.xz: %.img
	xz -T $(CPUS) $<

$(IMAGE):
	sudo ./scripts/make-image.sh
	mv build/system.img $(IMAGE)

image: $(IMAGE)
image-xz: $(IMAGE).xz

publish-image: $(IMAGE).xz
	rsync --progress $< $(IMG_PATH)/

publish:
	echo -n $(VERSION) > dist/latest.txt
	rsync $(RSYNC_OPTS) dist/$(PKG_NAME).tar.bz2 dist/latest.txt $(PUB_PATH)/

publish-beta:
	$(MAKE) push-beta

push-beta:
	rsync $(RSYNC_OPTS) dist/$(CUR_BETA_PKG_NAME).tar.bz2 dist/latest-beta.txt \
	  $(PUB_PATH)/

update:
	http_proxy= ./scripts/remote-firmware-update $(HOST) \
	  dist/$(PKG_NAME).tar.bz2
	@-tput sgr0 && echo # Fix terminal output

ssh-update:
	scp scripts/update-bbctrl dist/$(PKG_NAME).tar.bz2 $(HOST):/tmp/
	ssh -t $(HOST) "sudo /tmp/update-bbctrl /tmp/$(PKG_NAME).tar.bz2"

build/templates.pug: $(TEMPLS)
	mkdir -p build
	cat $(TEMPLS) >$@

node_modules: package.json
	npm install && touch node_modules

$(TARGET_DIR)/%: src/resources/%
	install -D $< $@

$(TARGET_DIR)/index.html: build/templates.pug
$(TARGET_DIR)/index.html: $(wildcard src/static/js/*)
$(TARGET_DIR)/index.html: $(wildcard src/static/css/*)
$(TARGET_DIR)/index.html: $(wildcard src/js/*)
$(TARGET_DIR)/index.html: $(wildcard src/stylus/*)
$(TARGET_DIR)/index.html: src/resources/config-template.json

$(TARGET_DIR)/%.html: src/pug/%.pug node_modules
	@mkdir -p $(shell dirname $@)
	$(PUG) -O pug-opts.js -P $< -o $(TARGET_DIR) || (rm -f $@; exit 1)

pylint:
	pylint -E $(shell find src/py -name \*.py | grep -v flycheck_)

jshint:
	./node_modules/jshint/bin/jshint --config jshint.json src/js/*.js

lint: pylint jshint

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
	rm -rf build html dist
	@for SUB in $(SUBPROJECTS); do \
	  $(MAKE) -C src/$$SUB clean; \
	done

dist-clean: clean
	rm -rf node_modules

.PHONY: all install clean tidy pkg arm-bin camotics lint pylint jshint
.PHONY: html resources dist-clean
