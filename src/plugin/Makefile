#!/usr/bin/make -f

export DISTRHO_NAMESPACE = DesktopDISTRHO
export DGL_NAMESPACE = DesktopDGL
export FILE_BROWSER_DISABLED = true
export NVG_FONT_TEXTURE_FLAGS = NVG_IMAGE_NEAREST

include ../DPF/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Project name, used for binaries

NAME = mod-desktop

# ---------------------------------------------------------------------------------------------------------------------
# Files to build

FILES_DSP = DesktopPlugin.cpp utils.cpp
FILES_UI = DesktopUI.cpp NanoButton.cpp utils.cpp

ifeq ($(MACOS),true)
FILES_UI += WebView.mm
else ifeq ($(WINDOWS),true)
FILES_UI += WebViewWin32.cpp
else
FILES_UI += WebViewX11.cpp
endif

# ---------------------------------------------------------------------------------------------------------------------
# Do some magic

DPF_BUILD_DIR = ../../build-plugin/build
DPF_TARGET_DIR = ../../build-plugin
USING_WEBVIEW = true

include ../DPF/Makefile.plugins.mk

BUILD_CXX_FLAGS += -DVERSION='"$(shell cat ../../VERSION)"'

BUILD_CXX_FLAGS += -pthread
LINK_FLAGS += -pthread

ifeq ($(MACOS),true)
LINK_FLAGS += -framework CoreFoundation -framework IOKit -framework WebKit
else ifeq ($(WINDOWS),true)
LINK_FLAGS += -lwinmm
else ifeq ($(LINUX),true)
BUILD_CXX_FLAGS += $(shell $(PKG_CONFIG) --cflags Qt5Core)
# -std=gnu++14
LINK_FLAGS += -ldl -lrt
endif

TARGETS = features au clap lv2_sep vst2 vst3

ifeq ($(WINDOWS),true)
BUILD_CXX_FLAGS += -I../CHOC
LINK_FLAGS += -lole32
$(BUILD_DIR)/WebViewWin32.cpp.o: BUILD_CXX_FLAGS += -std=gnu++17
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)
