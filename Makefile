#!/usr/bin/make -f

VERSION = $(shell cat VERSION)

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect build target

CC ?= gcc
TARGET_MACHINE := $(shell $(CC) -dumpmachine)

ifeq ($(PAWPAW_TARGET),macos)
MACOS = true
else ifeq ($(PAWPAW_TARGET),macos-universal-10.15)
MACOS = true
else ifeq ($(PAWPAW_TARGET),wasm)
WASM = true
else ifeq ($(PAWPAW_TARGET),win64)
WINDOWS = true
else
ifneq (,$(findstring linux,$(TARGET_MACHINE)))
LINUX = true
PAWPAW_TARGET = linux-$(shell uname -m)
else ifneq (,$(findstring apple,$(TARGET_MACHINE)))
MACOS = true
PAWPAW_TARGET = macos-universal-10.15
else ifneq (,$(findstring mingw,$(TARGET_MACHINE)))
WINDOWS = true
PAWPAW_TARGET = win64
else ifneq (,$(findstring wasm,$(TARGET_MACHINE)))
WASM = true
PAWPAW_TARGET = wasm
else
$(error unknown target, cannot continue)
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set binary file extensions as needed by JACK

ifeq ($(WINDOWS),true)
APP_EXT = .exe
SO_EXT = .dll
else
APP_EXT =
SO_EXT = .so
endif

# ---------------------------------------------------------------------------------------------------------------------
# utilities

# function to convert project name into buildroot var name
BUILDROOT_VAR = $(shell echo $(1) | tr a-z A-Z | tr - _)

# function to convert project name into stamp filename
PLUGIN_STAMP = build-plugin-stamps/$(1),$($(call BUILDROOT_VAR,$(1))_VERSION)

# makefile helpers
BLANK =
COMMA = ,
SPACE = $(BLANK) $(BLANK)

# ---------------------------------------------------------------------------------------------------------------------
# Set PawPaw environment, matching PawPaw/setup/env.sh

PAWPAW_DIR = $(HOME)/PawPawBuilds
PAWPAW_BUILDDIR = $(PAWPAW_DIR)/builds/$(PAWPAW_TARGET)
PAWPAW_PREFIX = $(PAWPAW_DIR)/targets/$(PAWPAW_TARGET)

# ---------------------------------------------------------------------------------------------------------------------
# List of files created by PawPaw bootstrap, to ensure we have run it at least once

BOOTSTRAP_FILES  = $(PAWPAW_PREFIX)/bin/cxfreeze
BOOTSTRAP_FILES += $(PAWPAW_PREFIX)/bin/jackd$(APP_EXT)
BOOTSTRAP_FILES += $(PAWPAW_PREFIX)/include/armadillo

# ---------------------------------------------------------------------------------------------------------------------
# List of files for mod-app packaging, often symlinks to the real files

TARGETS = build/lib-ui/libmod_utils$(SO_EXT)

ifeq ($(MACOS),true)
TARGETS += build/mod-app.app/Contents/Info.plist
TARGETS += build/mod-app.app/Contents/Frameworks/QtCore.framework
TARGETS += build/mod-app.app/Contents/Frameworks/QtGui.framework
TARGETS += build/mod-app.app/Contents/Frameworks/QtOpenGL.framework
TARGETS += build/mod-app.app/Contents/Frameworks/QtPrintSupport.framework
TARGETS += build/mod-app.app/Contents/Frameworks/QtSvg.framework
TARGETS += build/mod-app.app/Contents/Frameworks/QtWidgets.framework
TARGETS += build/mod-app.app/Contents/MacOS/jackd
TARGETS += build/mod-app.app/Contents/MacOS/jack/jack-session.conf
TARGETS += build/mod-app.app/Contents/MacOS/jack/jack_coreaudio.so
TARGETS += build/mod-app.app/Contents/MacOS/jack/jack_coremidi.so
TARGETS += build/mod-app.app/Contents/MacOS/jack/mod-host.so
TARGETS += build/mod-app.app/Contents/MacOS/jack/mod-midi-broadcaster.so
TARGETS += build/mod-app.app/Contents/MacOS/jack/mod-midi-merger.so
TARGETS += build/mod-app.app/Contents/MacOS/libjack.0.dylib
TARGETS += build/mod-app.app/Contents/MacOS/libjackserver.0.dylib
TARGETS += build/mod-app.app/Contents/MacOS/lib-screenshot/library.zip
TARGETS += build/mod-app.app/Contents/MacOS/lib-ui/library.zip
TARGETS += build/mod-app.app/Contents/MacOS/mod-app
TARGETS += build/mod-app.app/Contents/MacOS/mod-screenshot
TARGETS += build/mod-app.app/Contents/MacOS/mod-ui
TARGETS += build/mod-app.app/Contents/MacOS/mod
TARGETS += build/mod-app.app/Contents/MacOS/modtools
TARGETS += build/mod-app.app/Contents/PlugIns/generic/libqtuiotouchplugin.dylib
TARGETS += build/mod-app.app/Contents/PlugIns/iconengines/libqsvgicon.dylib
TARGETS += build/mod-app.app/Contents/PlugIns/imageformats/libqsvg.dylib
TARGETS += build/mod-app.app/Contents/PlugIns/LV2
TARGETS += build/mod-app.app/Contents/PlugIns/platforms/libqcocoa.dylib
TARGETS += build/mod-app.app/Contents/PlugIns/styles/libqmacstyle.dylib
TARGETS += build/mod-app.app/Contents/Resources/default.pedalboard
TARGETS += build/mod-app.app/Contents/Resources/html
TARGETS += build/mod-app.app/Contents/Resources/mod-hardware-descriptor.json
TARGETS += build/mod-app.app/Contents/Resources/mod-logo.icns
TARGETS += build/mod-app.app/Contents/Resources/VERSION
else
TARGETS += build/default.pedalboard
TARGETS += build/html
TARGETS += build/jackd$(APP_EXT)
TARGETS += build/jack/jack-session.conf
TARGETS += build/jack/mod-host$(SO_EXT)
TARGETS += build/jack/mod-midi-broadcaster$(SO_EXT)
TARGETS += build/jack/mod-midi-merger$(SO_EXT)
TARGETS += build/lib-screenshot/library.zip
TARGETS += build/lib-ui/library.zip
TARGETS += build/mod-app$(APP_EXT)
TARGETS += build/mod-screenshot$(APP_EXT)
TARGETS += build/mod-ui$(APP_EXT)
TARGETS += build/mod
TARGETS += build/modtools
TARGETS += build/mod-hardware-descriptor.json
TARGETS += build/VERSION
ifeq ($(WINDOWS),true)
TARGETS += build/jack/jack_portaudio.dll
TARGETS += build/jack/jack_winmme.dll
TARGETS += build/libjack64.dll
TARGETS += build/libjackserver64.dll
TARGETS += build/libpython3.8.dll
TARGETS += build/Qt5Core.dll
TARGETS += build/Qt5Gui.dll
TARGETS += build/Qt5Svg.dll
TARGETS += build/Qt5Widgets.dll
TARGETS += build/generic/qtuiotouchplugin.dll
TARGETS += build/iconengines/qsvgicon.dll
TARGETS += build/imageformats/qsvg.dll
TARGETS += build/platforms/qwindows.dll
TARGETS += build/styles/qwindowsvistastyle.dll
else
TARGETS += build/jack/alsa_midi.so
TARGETS += build/jack/jack_alsa.so
TARGETS += build/jack/jack_portaudio.so
TARGETS += build/jack/jack-session-alsamidi.conf
TARGETS += build/libjack.so.0
TARGETS += build/libjackserver.so.0
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set up plugins to build and ship inside mod-app

# List of plugin projects to build
PLUGINS  = abgate
PLUGINS += aidadsp-lv2
PLUGINS += artyfx
PLUGINS += bolliedelay
PLUGINS += caps-lv2
PLUGINS += carla-plugins
PLUGINS += die-plugins
PLUGINS += dpf-plugins
PLUGINS += dragonfly-reverb
PLUGINS += fluidplug
PLUGINS += fomp
PLUGINS += gxquack
PLUGINS += invada-lv2
PLUGINS += mod-ams-lv2
PLUGINS += mod-audio-mixers
PLUGINS += mod-convolution-loader
PLUGINS += mod-cv-plugins
PLUGINS += mod-distortion
PLUGINS += mod-mda-lv2
PLUGINS += mod-midi-utilities
PLUGINS += mod-pitchshifter
PLUGINS += mod-utilities
PLUGINS += modmeter
PLUGINS += modspectre
PLUGINS += neural-amp-modeler-lv2
PLUGINS += neuralrecord
PLUGINS += notes-lv2
PLUGINS += pitchtracking-series
# crashing macos https://github.com/moddevices/mod-app/actions/runs/6718888228/job/18259448918
# crashing macos https://github.com/moddevices/mod-app/actions/runs/6718888228/job/18259448741
# PLUGINS += screcord
PLUGINS += setbfree
PLUGINS += setbfree-mod
PLUGINS += sooperlooper-lv2
PLUGINS += shiro-plugins
PLUGINS += tap-lv2
PLUGINS += wolf-shaper
PLUGINS += x42-fat1
PLUGINS += x42-fil4
PLUGINS += x42-midifilter
PLUGINS += x42-midigen
PLUGINS += x42-stepseq
PLUGINS += x42-tinygain
PLUGINS += x42-xfade
PLUGINS += zam-plugins

# conflict with lv2-dev
# PLUGINS += lv2-examples

# include plugin projects for version and bundle list
include $(foreach PLUGIN,$(PLUGINS),mod-plugin-builder/plugins/package/$(PLUGIN)/$(PLUGIN).mk)

# list of unwanted bundles (for a variety of reasons)
UNWANTED_BUNDLES  = artyfx-bitta.lv2
UNWANTED_BUNDLES += b_whirl
UNWANTED_BUNDLES += mod-2voices.lv2
UNWANTED_BUNDLES += mod-bypass.lv2
UNWANTED_BUNDLES += mod-caps-AmpVTS.lv2
UNWANTED_BUNDLES += mod-caps-CabinetIV.lv2
UNWANTED_BUNDLES += mod-caps-CEO.lv2
UNWANTED_BUNDLES += mod-harmonizercs.lv2
UNWANTED_BUNDLES += mod-mda-Ambience.lv2
UNWANTED_BUNDLES += mod-mda-Degrade.lv2
UNWANTED_BUNDLES += mod-mda-DubDelay.lv2
UNWANTED_BUNDLES += mod-mda-Leslie.lv2
UNWANTED_BUNDLES += mod-mda-Overdrive.lv2
UNWANTED_BUNDLES += mod-mda-RePsycho.lv2
UNWANTED_BUNDLES += mod-mda-RingMod.lv2
UNWANTED_BUNDLES += mod-mda-RoundPan.lv2
UNWANTED_BUNDLES += sc_record.lv2
UNWANTED_BUNDLES += tap-autopan.lv2
UNWANTED_BUNDLES += tap-dynamics.lv2
UNWANTED_BUNDLES += tap-dynamics-st.lv2
UNWANTED_BUNDLES += tap-eqbw.lv2
UNWANTED_BUNDLES += tap-eq.lv2
UNWANTED_BUNDLES += tap-reverb.lv2
UNWANTED_BUNDLES += tap-sigmoid.lv2

# list of known good bundles
BUNDLES = $(filter-out $(UNWANTED_BUNDLES),$(foreach PLUGIN,$(PLUGINS),$($(call BUILDROOT_VAR,$(PLUGIN))_BUNDLES)))

# add plugins to build target
TARGETS += $(foreach PLUGIN,$(PLUGINS),$(call PLUGIN_STAMP,$(PLUGIN)))

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

clean:
	$(MAKE) clean -C mod-host
	$(MAKE) clean -C mod-ui/utils
	$(MAKE) clean -C systray
	rm -rf mod-midi-merger/build
	rm -rf build
	rm -rf build-plugin-stamps
	rm -rf build-screenshot
	rm -rf build-ui

plugins: $(foreach PLUGIN,$(PLUGINS),$(call PLUGIN_STAMP,$(PLUGIN)))

run: $(TARGETS)
	./utils/test.sh $(PAWPAW_TARGET)

version:
	@echo $(VERSION)

# ---------------------------------------------------------------------------------------------------------------------

macos:
	$(MAKE) PAWPAW_TARGET=macos-universal-10.15

macos-app:
	./utils/run.sh macos-universal-10.15 $(MAKE) -C systray

macos-plugins:
	$(MAKE) PAWPAW_TARGET=macos-universal-10.15 plugins

wasm:
	$(MAKE) PAWPAW_TARGET=wasm

wasm-plugins:
	$(MAKE) PAWPAW_TARGET=wasm plugins

win64:
	$(MAKE) PAWPAW_TARGET=win64

win64-app:
	./utils/run.sh win64 $(MAKE) -C systray

win64-plugins:
	$(MAKE) PAWPAW_TARGET=win64 plugins

# ---------------------------------------------------------------------------------------------------------------------

build/lib-ui/libmod_utils$(SO_EXT): mod-ui/utils/libmod_utils$(SO_EXT) build/lib-ui/library.zip
	ln -sf $(abspath $<) $@
	touch $@

# ---------------------------------------------------------------------------------------------------------------------

build/mod-app.app/Contents/Info.plist: utils/macos/Info.plist
	@mkdir -p build/mod-app.app/Contents
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/Frameworks/Qt%.framework: $(PAWPAW_PREFIX)/lib/Qt%.framework
	@mkdir -p build/mod-app.app/Contents/Frameworks
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/jackd: $(PAWPAW_PREFIX)/bin/jackd$(APP_EXT)
	@mkdir -p build/mod-app.app/Contents/MacOS
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/jack/jack-session.conf: utils/jack/jack-session.conf
	@mkdir -p build/mod-app.app/Contents/MacOS/jack
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/jack/jack_%.so: $(PAWPAW_PREFIX)/lib/jack/jack_%.so
	@mkdir -p build/mod-app.app/Contents/MacOS/jack
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/jack/mod-host.so: mod-host/mod-host.so
	@mkdir -p build/mod-app.app/Contents/MacOS/jack
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/jack/mod-midi-broadcaster.so: mod-midi-merger/build/mod-midi-broadcaster.so
	@mkdir -p build/mod-app.app/Contents/MacOS/jack
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/jack/mod-midi-merger.so: mod-midi-merger/build/mod-midi-merger.so
	@mkdir -p build/mod-app.app/Contents/MacOS/jack
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/libjack%: $(PAWPAW_PREFIX)/lib/libjack%
	@mkdir -p build/mod-app.app/Contents/MacOS
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/lib-ui/library.zip: build-ui/mod-ui$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath build-ui/lib) build/mod-app.app/Contents/MacOS/lib-ui
	touch $@

build/mod-app.app/Contents/MacOS/lib-screenshot/library.zip: build-screenshot/mod-screenshot$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath build-screenshot/lib) build/mod-app.app/Contents/MacOS/lib-screenshot
	touch $@

build/mod-app.app/Contents/MacOS/mod-app: systray/mod-app
	@mkdir -p build/mod-app.app/Contents/MacOS
	cp -v $(abspath $<) $@

build/mod-app.app/Contents/MacOS/mod-screenshot: build-screenshot/mod-screenshot
	@mkdir -p build/mod-app.app/Contents/MacOS
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/mod-ui: build-ui/mod-ui$(APP_EXT)
	@mkdir -p build/mod-app.app/Contents/MacOS
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/mod: mod-ui/mod
	@mkdir -p build/mod-app.app/Contents/MacOS/
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/MacOS/modtools: mod-ui/modtools
	@mkdir -p build/mod-app.app/Contents/MacOS/
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/PlugIns/generic/libq%.dylib: $(PAWPAW_PREFIX)/lib/qt5/plugins/generic/libq%.dylib
	@mkdir -p build/mod-app.app/Contents/PlugIns/generic
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/PlugIns/iconengines/libq%.dylib: $(PAWPAW_PREFIX)/lib/qt5/plugins/iconengines/libq%.dylib
	@mkdir -p build/mod-app.app/Contents/PlugIns/iconengines
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/PlugIns/imageformats/libq%.dylib: $(PAWPAW_PREFIX)/lib/qt5/plugins/imageformats/libq%.dylib
	@mkdir -p build/mod-app.app/Contents/PlugIns/imageformats
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/PlugIns/LV2:
	@mkdir -p build/mod-app.app/Contents/PlugIns
	@mkdir -p build/plugins
	ln -sf $(abspath build/plugins) $@

build/mod-app.app/Contents/PlugIns/platforms/libq%.dylib: $(PAWPAW_PREFIX)/lib/qt5/plugins/platforms/libq%.dylib
	@mkdir -p build/mod-app.app/Contents/PlugIns/platforms
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/PlugIns/styles/libq%.dylib: $(PAWPAW_PREFIX)/lib/qt5/plugins/styles/libq%.dylib
	@mkdir -p build/mod-app.app/Contents/PlugIns/styles
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/Resources/default.pedalboard: mod-ui/default.pedalboard
	@mkdir -p build/mod-app.app/Contents/Resources
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/Resources/html: mod-ui/html
	@mkdir -p build/mod-app.app/Contents/Resources
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/Resources/mod-hardware-descriptor.json: utils/macos/mod-hardware-descriptor.json
	@mkdir -p build/mod-app.app/Contents/Resources
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/Resources/mod-logo.icns: systray/mod-logo.icns
	@mkdir -p build/mod-app.app/Contents/Resources
	ln -sf $(abspath $<) $@

build/mod-app.app/Contents/Resources/VERSION: VERSION
	@mkdir -p build/mod-app.app/Contents/Resources
	ln -sf $(abspath $<) $@

# ---------------------------------------------------------------------------------------------------------------------

build/default.pedalboard: mod-ui/default.pedalboard
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/html: mod-ui/html
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/jackd$(APP_EXT): $(PAWPAW_PREFIX)/bin/jackd$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/jack/%.conf: utils/jack/%.conf
	@mkdir -p build/jack
	ln -sf $(abspath $<) $@

build/jack/mod-host$(SO_EXT): mod-host/mod-host.so
	@mkdir -p build/jack
	ln -sf $(abspath $<) $@

build/jack/mod-midi-broadcaster$(SO_EXT): mod-midi-merger/build/mod-midi-broadcaster$(SO_EXT)
	@mkdir -p build/jack
	ln -sf $(abspath $<) $@

build/jack/mod-midi-merger$(SO_EXT): mod-midi-merger/build/mod-midi-merger$(SO_EXT)
	@mkdir -p build/jack
	ln -sf $(abspath $<) $@

build/lib: build-ui/mod-ui$(APP_EXT)
	rm -f $@
	ln -sf $(abspath build-ui/lib) $@
	touch $@

build/mod-app$(APP_EXT): systray/mod-app$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/mod-ui$(APP_EXT): build-ui/mod-ui$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/mod-screenshot$(APP_EXT): build-screenshot/mod-screenshot$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/mod: mod-ui/mod
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/modtools: mod-ui/modtools
	@mkdir -p build
	ln -sf $(abspath $<) $@

ifeq ($(WINDOWS),true)
build/mod-hardware-descriptor.json: utils/win64/mod-hardware-descriptor.json
	@mkdir -p build
	ln -sf $(abspath $<) $@
else
build/mod-hardware-descriptor.json: utils/linux/mod-hardware-descriptor.json
	@mkdir -p build
	ln -sf $(abspath $<) $@
endif

build/VERSION: VERSION
	@mkdir -p build
	ln -sf $(abspath $<) $@

# ---------------------------------------------------------------------------------------------------------------------

build/jack/alsa_midi.so: $(PAWPAW_PREFIX)/lib/jack/alsa_midi.so
	@mkdir -p build/jack
	ln -sf $(abspath $<) $@

build/jack/jack_%: $(PAWPAW_PREFIX)/lib/jack/jack_%
	@mkdir -p build/jack
	ln -sf $(abspath $<) $@

build/lib-ui/library.zip: build-ui/mod-ui$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath build-ui/lib) build/lib-ui
	touch $@

build/lib-screenshot/library.zip: build-screenshot/mod-screenshot$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath build-screenshot/lib) build/lib-screenshot
	touch $@

build/libjack%: $(PAWPAW_PREFIX)/lib/libjack%
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/libpython%: $(PAWPAW_PREFIX)/bin/libpython%
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/Qt5%.dll: $(PAWPAW_PREFIX)/bin/Qt5%.dll
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/generic/q%.dll: $(PAWPAW_PREFIX)/lib/qt5/plugins/generic/q%.dll
	@mkdir -p build/generic
	ln -sf $(abspath $<) $@

build/iconengines/q%.dll: $(PAWPAW_PREFIX)/lib/qt5/plugins/iconengines/q%.dll
	@mkdir -p build/iconengines
	ln -sf $(abspath $<) $@

build/imageformats/q%.dll: $(PAWPAW_PREFIX)/lib/qt5/plugins/imageformats/q%.dll
	@mkdir -p build/imageformats
	ln -sf $(abspath $<) $@

build/platforms/q%.dll: $(PAWPAW_PREFIX)/lib/qt5/plugins/platforms/q%.dll
	@mkdir -p build/platforms
	ln -sf $(abspath $<) $@

build/styles/q%.dll: $(PAWPAW_PREFIX)/lib/qt5/plugins/styles/q%.dll
	@mkdir -p build/styles
	ln -sf $(abspath $<) $@

# ---------------------------------------------------------------------------------------------------------------------

build-screenshot/mod-screenshot$(APP_EXT): utils/cxfreeze/mod-screenshot.py utils/cxfreeze/mod-screenshot-setup.py $(BOOTSTRAP_FILES)
	./utils/run.sh $(PAWPAW_TARGET) python3 utils/cxfreeze/mod-screenshot.py build_exe
	touch $@

# ---------------------------------------------------------------------------------------------------------------------

build-ui/mod-ui$(APP_EXT): utils/cxfreeze/mod-ui.py utils/cxfreeze/mod-ui-setup.py $(BOOTSTRAP_FILES)
	./utils/run.sh $(PAWPAW_TARGET) python3 utils/cxfreeze/mod-ui.py build_exe
	touch $@

mod-ui/utils/libmod_utils$(SO_EXT): $(BOOTSTRAP_FILES) mod-ui/utils/utils.h mod-ui/utils/utils_jack.cpp mod-ui/utils/utils_lilv.cpp
	./utils/run.sh $(PAWPAW_TARGET) $(MAKE) MODAPP=1 -C mod-ui/utils

# ---------------------------------------------------------------------------------------------------------------------

mod-host/mod-host.so: mod-host/src/*.c mod-host/src/*.h mod-host/src/monitor/*.c mod-host/src/monitor/*.h $(BOOTSTRAP_FILES)
	./utils/run.sh $(PAWPAW_TARGET) $(MAKE) MODAPP=1 SKIP_READLINE=1 SKIP_FFTW335=1 -C mod-host

# ---------------------------------------------------------------------------------------------------------------------

mod-midi-merger/build/mod-midi-broadcaster$(SO_EXT): mod-midi-merger/build/mod-midi-merger-standalone$(APP_EXT)
	touch $@

mod-midi-merger/build/mod-midi-merger$(SO_EXT): mod-midi-merger/build/mod-midi-merger-standalone$(APP_EXT)
	touch $@

mod-midi-merger/build/mod-midi-merger-standalone$(APP_EXT): mod-midi-merger/build/Makefile mod-midi-merger/src/*.c mod-midi-merger/src/*.h
	./utils/run.sh $(PAWPAW_TARGET) cmake --build mod-midi-merger/build
	touch $@

mod-midi-merger/build/Makefile: $(BOOTSTRAP_FILES)
	./utils/run.sh $(PAWPAW_TARGET) cmake -S mod-midi-merger -B mod-midi-merger/build

# ---------------------------------------------------------------------------------------------------------------------

systray/mod-app$(APP_EXT): systray/main.cpp systray/mod-app.hpp systray/mod-app.ui systray/widgets.hpp
	./utils/run.sh $(PAWPAW_TARGET) $(MAKE) -C systray

# ---------------------------------------------------------------------------------------------------------------------

define BUILD_PLUGIN
	@mkdir -p build/plugins
	./utils/plugin-builder/plugin-builder.sh $(PAWPAW_TARGET) $(1)
	$(foreach BUNDLE,$(filter $(BUNDLES),$($(call BUILDROOT_VAR,$(1))_BUNDLES)),\
		rm -f build/plugins/$(BUNDLE);\
		ln -s $(abspath $(PAWPAW_PREFIX)/lib/lv2/$(BUNDLE)) build/plugins/$(BUNDLE);\
	)
endef

build-plugin-stamps/%: $(BOOTSTRAP_FILES)
	@mkdir -p build-plugin-stamps
	$(call BUILD_PLUGIN,$(firstword $(subst $(COMMA), ,$*)))
	touch $@

# ---------------------------------------------------------------------------------------------------------------------

bootstrap:
	./PawPaw/bootstrap-mod.sh $(PAWPAW_TARGET)

$(PAWPAW_PREFIX)/bin/%:
	./PawPaw/bootstrap-mod.sh $(PAWPAW_TARGET)

$(PAWPAW_PREFIX)/lib/jack/alsa_midi.so:
	./PawPaw/bootstrap-mod.sh $(PAWPAW_TARGET)

$(PAWPAW_PREFIX)/lib/lib%:
	./PawPaw/bootstrap-mod.sh $(PAWPAW_TARGET)

$(PAWPAW_PREFIX)/include/%:
	./PawPaw/bootstrap-mod.sh $(PAWPAW_TARGET)

# ---------------------------------------------------------------------------------------------------------------------
