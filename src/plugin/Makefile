#!/usr/bin/make -f

export DISTRHO_NAMESPACE = DesktopDISTRHO
export DGL_NAMESPACE = DesktopDGL
export NVG_FONT_TEXTURE_FLAGS = NVG_IMAGE_NEAREST
export USE_WEB_VIEW = true

include ../DPF/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Project name, used for binaries

NAME = mod-desktop

# ---------------------------------------------------------------------------------------------------------------------
# Files to build

FILES_DSP = \
	DesktopPlugin.cpp \
	utils.cpp \
	zita-resampler/resampler.cc \
	zita-resampler/resampler-table.cc

FILES_UI = DesktopUI.cpp NanoButton.cpp utils.cpp

# ---------------------------------------------------------------------------------------------------------------------
# Do some magic

DPF_BUILD_DIR = ../../build-plugin/build
DPF_TARGET_DIR = ../../build-plugin
USING_WEBVIEW = true

include ../DPF/Makefile.plugins.mk

# VERSION is compiled in (-DVERSION), a change must rebuild
$(OBJS_DSP) $(OBJS_UI): ../../VERSION

TARGETS = features jack au clap lv2_sep vst2 vst3

BUILD_CXX_FLAGS += -DVERSION='"$(shell cat ../../VERSION)"'
# the app/plugin link file is keyed on the release line (0.14), not the build (0.14.N),
# so a plugin from one build still finds the app from another build of the same line
BUILD_CXX_FLAGS += -DVERSION_LINE='"$(shell cut -d. -f1-2 ../../VERSION)"'
BUILD_CXX_FLAGS += -pthread
LINK_FLAGS += -pthread

ifeq ($(MACOS),true)
LINK_FLAGS += -framework CoreFoundation -framework IOKit
else ifeq ($(WINDOWS),true)
LINK_FLAGS += -lole32 -luuid -lwinmm
else
LINK_FLAGS += -ldl -lrt
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)
