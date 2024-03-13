#!/usr/bin/make -f

include ../DPF/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Project name, used for binaries

NAME = MOD-Desktop

# ---------------------------------------------------------------------------------------------------------------------
# Files to build

FILES_DSP = DesktopPlugin.cpp utils.cpp
FILES_UI = DesktopUI.cpp NanoButton.cpp

ifeq ($(MACOS),true)
FILES_UI += WebView.mm
else ifeq ($(LINUX),true)
FILES_UI += WebViewX11.cpp
endif

# ---------------------------------------------------------------------------------------------------------------------
# Do some magic

DPF_BUILD_DIR = ../../build-plugin/build
DPF_TARGET_DIR = ../../build-plugin

include ../DPF/Makefile.plugins.mk

BUILD_CXX_FLAGS += -DVERSION='"$(shell cat ../../VERSION)"'

ifeq ($(MACOS),true)
LINK_FLAGS += -framework IOKit -framework WebKit
else ifeq ($(WINDOWS),true)
LINK_FLAGS += -lwinmm
else ifeq ($(LINUX),true)
LINK_FLAGS += -ldl
endif

TARGETS = jack vst2

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(LINUX),true)

TARGETS += $(TARGET_DIR)/MOD-Desktop-WebView

$(TARGET_DIR)/MOD-Desktop-WebView: $(BUILD_DIR)/WebViewQt.cpp.o
	$(CXX) $^ $(LINK_FLAGS) $(shell $(PKG_CONFIG) --libs Qt5WebEngineWidgets) -o $@

$(BUILD_DIR)/WebViewQt.cpp.o: BUILD_CXX_FLAGS += -std=gnu++14 $(shell $(PKG_CONFIG) --cflags Qt5WebEngineWidgets)

include $(BUILD_DIR)/WebViewQt.cpp.d

endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)
