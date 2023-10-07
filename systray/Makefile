#!/usr/bin/make -f

CC ?= gcc
CXX ?= g++

CXXFLAGS += -fPIC -std=gnu++14

TARGET_MACHINE := $(shell $(CC) -dumpmachine)

# ---------------------------------------------------------------------------------------------------------------------

ifneq (,$(findstring linux,$(TARGET_MACHINE)))
LINUX = true
WINDOWS = false
PAWPAW_TARGET = linux-$(shell uname -m)
else ifneq (,$(findstring apple,$(TARGET_MACHINE)))
MACOS = true
WINDOWS = false
PAWPAW_TARGET = macos-universal-10.15
else ifneq (,$(findstring mingw,$(TARGET_MACHINE)))
WINDOWS = true
PAWPAW_TARGET = win64
else ifneq (,$(findstring wasm,$(TARGET_MACHINE)))
WASM = true
WINDOWS = false
PAWPAW_TARGET = wasm
else
$(error unknown target, cannot continue)
endif

# ---------------------------------------------------------------------------------------------------------------------

PAWPAW_DIR = ~/PawPawBuilds
PAWPAW_PREFIX = $(PAWPAW_DIR)/targets/$(PAWPAW_TARGET)

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS),true)
TARGET = mod-app.app/Contents/MacOS/mod-app
else ifeq ($(WINDOWS),true)
TARGET = mod-app.exe
EXTRAS = Qt5Core.dll Qt5Gui.dll Qt5Svg.dll Qt5Widgets.dll
CXXFLAGS += $(shell pkg-config --cflags portaudio-2.0)
LDFLAGS  += $(shell pkg-config --libs portaudio-2.0)
LDFLAGS  += -Wl,-subsystem,windows
WINDRES   = $(subst gcc,windres,$(CC))
else
TARGET = mod-app
endif

ifeq ($(shell pkg-config --exists Qt5Core Qt5Gui Qt5Svg Qt5Widgets && echo true)$(WINDOWS),truefalse)
QT5_HOSTBINS = $(shell pkg-config --variable=host_bins Qt5Core)
QT5_FLAGS    = $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Svg Qt5Widgets)
QT5_LIBS     = $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Svg Qt5Widgets)
else
QT5_PREFIX   = $(shell pkg-config --variable=prefix Qt5OpenGLExtensions)
QT5_HOSTBINS = $(QT5_PREFIX)/bin
QT5_FLAGS    = -DQT_CORE_LIB -DQT_GUI_LIB -DQT_SVG_LIB -DQT_WIDGETS_LIB -I$(QT5_PREFIX)/include/qt5
ifeq ($(MACOS),true)
QT5_LIBS     = -F$(QT5_PREFIX)/lib -framework QtCore -framework QtGui -framework QtSvg -framework QtWidgets
else
QT5_LIBS     = -L$(QT5_PREFIX)/lib -lQt5Core -lQt5Gui -lQt5Svg -lQt5Widgets
endif
endif

OBJS  = main.cpp.o

ifeq ($(WINDOWS),true)
OBJS += mod-app.rc.o
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGET) $(EXTRAS)

clean:
	rm -f $(TARGET) $(EXTRAS) qrc_mod-app.hpp ui_mod-app.hpp

$(TARGET): $(OBJS)
	@$(shell mkdir -p $(@D))
	$(CXX) $^ $(LDFLAGS) $(QT5_LIBS) -o $@

main.cpp.o: main.cpp mod-app.hpp qrc_mod-app.hpp ui_mod-app.hpp
	$(CXX) $< $(CXXFLAGS) $(QT5_FLAGS) -c -o $@

mod-app.rc.o: mod-app.rc mod-logo.ico
	$(WINDRES) $< -O coff -o $@

qrc_mod-app.hpp: mod-app.qrc
	$(QT5_HOSTBINS)/rcc $< -o $@

ui_mod-app.hpp: mod-app.ui
	$(QT5_HOSTBINS)/uic $< -o $@

Qt5%.dll: $(PAWPAW_PREFIX)/bin/Qt5%.dll
	ln -sf $< $@

# ---------------------------------------------------------------------------------------------------------------------
