#!/usr/bin/make -f

VERSION = 0.0.1

CC ?= gcc
TARGET_MACHINE := $(shell $(CC) -dumpmachine)

ifeq ($(PAWPAW_TARGET),macos-universal-10.15)
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

ifeq ($(WINDOWS),true)
APP_EXT = .exe
SO_EXT = .dll
else
APP_EXT =
SO_EXT = .so
endif

# ---------------------------------------------------------------------------------------------------------------------

PAWPAW_DIR = ~/PawPawBuilds
PAWPAW_PREFIX = $(PAWPAW_DIR)/targets/$(PAWPAW_TARGET)

BOOTSTRAP_FILES = \
	$(PAWPAW_PREFIX)/bin/cxfreeze

# ---------------------------------------------------------------------------------------------------------------------

TARGETS = \
	build/jackd$(APP_EXT) \
	build/mod-screenshot$(APP_EXT) \
	build/mod-ui$(APP_EXT) \
	build/jack/jack-session.conf \
	build/jack/mod-host$(SO_EXT) \
	build/jack/mod-midi-broadcaster$(SO_EXT) \
	build/jack/mod-midi-merger$(SO_EXT) \
	build/lib/libmod_utils$(SO_EXT) \
	build/default.pedalboard \
	build/html \
	build/mod \
	build/modtools

ifeq ($(MACOS),true)
TARGETS += build/libjack.0.dylib
TARGETS += build/libjackserver.0.dylib
TARGETS += build/jack/jack_coreaudio.so
TARGETS += build/jack/jack_coremidi.so
else ifeq ($(WINDOWS),true)
TARGETS += build/libjack64.dll
TARGETS += build/libjackserver64.dll
TARGETS += build/libpython3.8.dll
TARGETS += build/jack/jack_portaudio.dll
TARGETS += build/jack/jack_winmme.dll
TARGETS += build/mod-app.exe
TARGETS += build/Qt5Core.dll
TARGETS += build/Qt5Gui.dll
TARGETS += build/Qt5Svg.dll
TARGETS += build/Qt5Widgets.dll
TARGETS += build/bearer/qgenericbearer.dll
TARGETS += build/generic/qtuiotouchplugin.dll
TARGETS += build/iconengines/qsvgicon.dll
TARGETS += build/imageformats/qsvg.dll
TARGETS += build/platforms/qwindows.dll
TARGETS += build/styles/qwindowsvistastyle.dll
else
TARGETS += build/libjack.so.0
TARGETS += build/libjackserver.so.0
TARGETS += build/jack/jack_alsa.so
endif

# ---------------------------------------------------------------------------------------------------------------------

# FIXME *.so extension
BUNDLES  = abGate.lv2
BUNDLES += artyfx.lv2
BUNDLES += carla-files.lv2
ifneq ($(MACOS),true)
BUNDLES += DragonflyEarlyReflections.lv2
BUNDLES += DragonflyHallReverb.lv2
BUNDLES += DragonflyPlateReverb.lv2
BUNDLES += DragonflyRoomReverb.lv2
# FIXME *.so extension
BUNDLES += fil4.lv2
endif
BUNDLES += Black_Pearl_4A.lv2
BUNDLES += Black_Pearl_4B.lv2
BUNDLES += Black_Pearl_5.lv2
BUNDLES += FluidBass.lv2
BUNDLES += FluidBrass.lv2
BUNDLES += FluidChromPerc.lv2
BUNDLES += FluidDrums.lv2
BUNDLES += FluidEnsemble.lv2
BUNDLES += FluidEthnic.lv2
BUNDLES += FluidGuitars.lv2
BUNDLES += FluidOrgans.lv2
BUNDLES += FluidPercussion.lv2
BUNDLES += FluidPianos.lv2
BUNDLES += FluidPipes.lv2
BUNDLES += FluidReeds.lv2
BUNDLES += FluidSoundFX.lv2
BUNDLES += FluidStrings.lv2
BUNDLES += FluidSynthFX.lv2
BUNDLES += FluidSynthLeads.lv2
BUNDLES += FluidSynthPads.lv2
BUNDLES += Red_Zeppelin_4.lv2
BUNDLES += Red_Zeppelin_5.lv2
ifneq ($(MACOS),true)
# FIXME needs python2
# BUNDLES += fomp.lv2
endif
BUNDLES += Kars.lv2
ifneq ($(MACOS),true)
# FIXME *.so extension
BUNDLES += midifilter.lv2
endif
BUNDLES += midigen.lv2
# FIXME *.so extension
BUNDLES += mod-bpf.lv2
BUNDLES += MOD-CabinetLoader.lv2
BUNDLES += MOD-ConvolutionLoader.lv2
# FIXME *.so extension
BUNDLES += mod-gain.lv2
# FIXME *.so extension
BUNDLES += mod-gain2x2.lv2
# FIXME *.so extension
BUNDLES += mod-hpf.lv2
BUNDLES += mod-mda-Ambience.lv2
BUNDLES += mod-mda-BeatBox.lv2
BUNDLES += mod-mda-Degrade.lv2
BUNDLES += mod-mda-Detune.lv2
BUNDLES += mod-mda-DubDelay.lv2
BUNDLES += mod-mda-DX10.lv2
BUNDLES += mod-mda-EPiano.lv2
BUNDLES += mod-mda-JX10.lv2
BUNDLES += mod-mda-Leslie.lv2
BUNDLES += mod-mda-Piano.lv2
BUNDLES += mod-mda-RePsycho.lv2
BUNDLES += mod-mda-RingMod.lv2
BUNDLES += mod-mda-RoundPan.lv2
BUNDLES += mod-mda-Shepard.lv2
BUNDLES += mod-mda-SubSynth.lv2
BUNDLES += mod-mda-ThruZero.lv2
BUNDLES += mod-mda-Vocoder.lv2
ifneq ($(MACOS),true)
# FIXME *.so extension
BUNDLES += modmeter.lv2
# FIXME *.so extension
BUNDLES += modspectre.lv2
endif
BUNDLES += MVerb.lv2
BUNDLES += Nekobi.lv2
BUNDLES += neural_amp_modeler.lv2
ifneq ($(MACOS),true)
BUNDLES += notes.lv2
endif
BUNDLES += PingPongPan.lv2
ifneq ($(MACOS),true)
# FIXME plugin binary missing (win32 RUNTIME vs LIBRARY)
BUNDLES += rt-neural-generic.lv2
endif
# FIXME *.so extension
BUNDLES += tinygain.lv2
BUNDLES += wolf-shaper.lv2

# TODO check
BUNDLES += Harmless.lv2
BUNDLES += Larynx.lv2
BUNDLES += Modulay.lv2
BUNDLES += Shiroverb.lv2


# TODO build fails
# BUNDLES += mod-ams.lv2
# BUNDLES += mod-cv-control.lv2
# BUNDLES += sooperlooper.lv2
# BUNDLES += sooperlooper-2x2.lv2

PLUGINS = $(BUNDLES:%=build/plugins/%)

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

clean:
	$(MAKE) clean -C mod-host
	$(MAKE) clean -C mod-ui/utils

plugins: $(PLUGINS)

run: $(TARGETS)
	./utils/test.sh $(PAWPAW_TARGET)

version:
	@echo $(VERSION)

# ---------------------------------------------------------------------------------------------------------------------

macos:
	$(MAKE) PAWPAW_TARGET=macos-universal-10.15

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

build/jackd$(APP_EXT): $(PAWPAW_PREFIX)/bin/jackd$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/Qt5%.dll: $(PAWPAW_PREFIX)/bin/Qt5%.dll
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/bearer/q%.dll: $(PAWPAW_PREFIX)/lib/qt5/plugins/bearer/q%.dll
	@mkdir -p build/bearer
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

build/libjack%: $(PAWPAW_PREFIX)/lib/libjack%
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/libpython%: $(PAWPAW_PREFIX)/bin/libpython%
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/jack/jack_%: $(PAWPAW_PREFIX)/lib/jack/jack_%
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

build/lib/libmod_utils$(SO_EXT): mod-ui/utils/libmod_utils.so
	@mkdir -p build/lib
	ln -sf $(abspath $<) $@

build/mod-app$(APP_EXT): systray/mod-app$(APP_EXT)
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/default.pedalboard: mod-ui/default.pedalboard
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/html: mod-ui/html
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/mod: mod-ui/mod
	@mkdir -p build
	ln -sf $(abspath $<) $@

build/modtools: mod-ui/modtools
	@mkdir -p build
	ln -sf $(abspath $<) $@

# ---------------------------------------------------------------------------------------------------------------------

build/jack/jack-session.conf: utils/jack-session.conf
	@mkdir -p build/jack
	ln -sf $(abspath $<) $@

build/mod-screenshot$(APP_EXT): utils/mod-screenshot.py $(BOOTSTRAP_FILES)
	./utils/run.sh $(PAWPAW_TARGET) python3 utils/mod-screenshot.py build_exe
	touch $@

build/mod-ui$(APP_EXT): utils/mod-ui.py utils/mod-ui-wrapper.py $(BOOTSTRAP_FILES)
	./utils/run.sh $(PAWPAW_TARGET) python3 utils/mod-ui.py build_exe
	touch $@

mod-host/mod-host.so: $(BOOTSTRAP_FILES)
	./utils/run.sh $(PAWPAW_TARGET) $(MAKE) SKIP_READLINE=1 SKIP_FFTW335=1 -C mod-host

mod-midi-merger/build/mod-midi-broadcaster$(SO_EXT): mod-midi-merger/build/mod-midi-merger-standalone$(APP_EXT)
	touch $@

mod-midi-merger/build/mod-midi-merger$(SO_EXT): mod-midi-merger/build/mod-midi-merger-standalone$(APP_EXT)
	touch $@

mod-midi-merger/build/mod-midi-merger-standalone$(APP_EXT): mod-midi-merger/build/Makefile
	./utils/run.sh $(PAWPAW_TARGET) cmake --build mod-midi-merger/build

mod-midi-merger/build/Makefile: $(BOOTSTRAP_FILES)
	./utils/run.sh $(PAWPAW_TARGET) cmake -S mod-midi-merger -B mod-midi-merger/build

mod-ui/utils/libmod_utils.so: $(BOOTSTRAP_FILES) mod-ui/utils/utils.h mod-ui/utils/utils_jack.cpp mod-ui/utils/utils_lilv.cpp
	./utils/run.sh $(PAWPAW_TARGET) $(MAKE) -C mod-ui/utils

systray/mod-app$(APP_EXT): systray/main.cpp systray/mod-app.hpp
	./utils/run.sh $(PAWPAW_TARGET) $(MAKE) -C systray

# ---------------------------------------------------------------------------------------------------------------------

build/plugins/%: $(PAWPAW_PREFIX)/lib/lv2/%/manifest.ttl
	@mkdir -p build/plugins
	ln -sf $(subst /manifest.ttl,,$(abspath $<)) $@

$(PAWPAW_PREFIX)/lib/lv2/abGate.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) abgate

$(PAWPAW_PREFIX)/lib/lv2/rt-neural-generic.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) aidadsp-lv2

$(PAWPAW_PREFIX)/lib/lv2/artyfx.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) artyfx

$(PAWPAW_PREFIX)/lib/lv2/carla-files.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) carla-plugins

$(PAWPAW_PREFIX)/lib/lv2/Kars.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) dpf-plugins

$(PAWPAW_PREFIX)/lib/lv2/MVerb.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) dpf-plugins

$(PAWPAW_PREFIX)/lib/lv2/Nekobi.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) dpf-plugins

$(PAWPAW_PREFIX)/lib/lv2/PingPongPan.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) dpf-plugins

$(PAWPAW_PREFIX)/lib/lv2/Dragonfly%/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) dragonfly-reverb

$(PAWPAW_PREFIX)/lib/lv2/Black_Pearl_%/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) fluidplug

$(PAWPAW_PREFIX)/lib/lv2/Fluid%/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) fluidplug

$(PAWPAW_PREFIX)/lib/lv2/Red_Zeppelin_%/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) fluidplug

$(PAWPAW_PREFIX)/lib/lv2/fomp.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) fomp

$(PAWPAW_PREFIX)/lib/lv2/MOD-CabinetLoader.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-convolution-loader

$(PAWPAW_PREFIX)/lib/lv2/MOD-ConvolutionLoader.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-convolution-loader

$(PAWPAW_PREFIX)/lib/lv2/mod-mda-%/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-mda-lv2

$(PAWPAW_PREFIX)/lib/lv2/mod-bpf.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-utilities

$(PAWPAW_PREFIX)/lib/lv2/mod-hpf.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-utilities

$(PAWPAW_PREFIX)/lib/lv2/mod-gain.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-utilities

$(PAWPAW_PREFIX)/lib/lv2/mod-gain2x2.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-utilities

$(PAWPAW_PREFIX)/lib/lv2/modmeter.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) modmeter

$(PAWPAW_PREFIX)/lib/lv2/modspectre.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) modspectre

$(PAWPAW_PREFIX)/lib/lv2/neural_amp_modeler.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) neural-amp-modeler-lv2

$(PAWPAW_PREFIX)/lib/lv2/notes.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) notes-lv2

$(PAWPAW_PREFIX)/lib/lv2/fil4.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) x42-fil4

$(PAWPAW_PREFIX)/lib/lv2/midifilter.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) x42-midifilter

$(PAWPAW_PREFIX)/lib/lv2/midigen.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) x42-midigen

$(PAWPAW_PREFIX)/lib/lv2/Harmless.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) shiro-plugins

$(PAWPAW_PREFIX)/lib/lv2/Larynx.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) shiro-plugins

$(PAWPAW_PREFIX)/lib/lv2/Modulay.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) shiro-plugins

$(PAWPAW_PREFIX)/lib/lv2/Shiroverb.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) shiro-plugins

$(PAWPAW_PREFIX)/lib/lv2/sooperlooper.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) sooperlooper-lv2

$(PAWPAW_PREFIX)/lib/lv2/sooperlooper-2x2.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) sooperlooper-lv2

$(PAWPAW_PREFIX)/lib/lv2/tinygain.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) x42-tinygain

$(PAWPAW_PREFIX)/lib/lv2/wolf-shaper.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) wolf-shaper

$(PAWPAW_PREFIX)/lib/lv2/mod-ams.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-ams-lv2

$(PAWPAW_PREFIX)/lib/lv2/mod-cv-control.lv2/manifest.ttl: $(BOOTSTRAP_FILES)
	./utils/plugin-builder.sh $(PAWPAW_TARGET) mod-cv-plugins

# ---------------------------------------------------------------------------------------------------------------------

$(PAWPAW_PREFIX)/bin/%:
	./PawPaw/bootstrap-mod.sh $(PAWPAW_TARGET)

$(PAWPAW_PREFIX)/lib/lib%:
	./PawPaw/bootstrap-mod.sh $(PAWPAW_TARGET)

# ---------------------------------------------------------------------------------------------------------------------
