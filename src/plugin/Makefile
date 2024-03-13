#!/usr/bin/make -f

include ../DPF/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Project name, used for binaries

NAME = MOD-Desktop

# ---------------------------------------------------------------------------------------------------------------------
# Files to build

FILES_DSP = DesktopPlugin.cpp
FILES_UI = DesktopUI.cpp

ifeq ($(MACOS),true)
FILES_UI += WebView.mm
endif

# ---------------------------------------------------------------------------------------------------------------------
# Do some magic

DPF_BUILD_DIR = ../../build-plugin/build
DPF_TARGET_DIR = ../../build-plugin

include ../DPF/Makefile.plugins.mk

ifeq ($(MACOS),true)
LINK_FLAGS += -framework WebKit
else ifeq ($(WINDOWS),true)
LINK_FLAGS += -lwinmm
endif

all: jack vst2

# ---------------------------------------------------------------------------------------------------------------------
