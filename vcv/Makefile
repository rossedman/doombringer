# VCV Rack 2 plugin. Point RACK_DIR at an unpacked Rack SDK:
#   make RACK_DIR=~/Rack-SDK install
# Cross-compile for Intel Macs from an Apple silicon Mac (with the mac-x64 SDK):
#   make RACK_DIR=~/Rack-SDK-mac-x64 CROSS_COMPILE=x86_64-apple-darwin dist
RACK_DIR ?= ../../Rack-SDK

# No directory of ours goes on the include path: on case-insensitive filesystems
# (macOS, Windows) dsp/Math.hpp shadows the SDK's <math.hpp>, and the root's VERSION
# shadows libc++'s <version>. Sources include the DSP by relative path instead.
# The SDK adds -std=c++11 after CXXFLAGS; EXTRA_CXXFLAGS is appended last, so this wins.
EXTRA_CXXFLAGS += -std=c++17

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res/Doombringer.svg
DISTRIBUTABLES += LICENSE

include $(RACK_DIR)/plugin.mk

# The SDK copies distributables by their relative path, so ../LICENSE would land
# outside the package. Copy it in next to plugin.json first.
LICENSE: ../LICENSE
	cp $< $@

dist: LICENSE
