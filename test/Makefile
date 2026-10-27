CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wshadow -Werror=float-conversion
BUILD    := build

$(BUILD)/host_tests: host_tests.cpp patches.hpp $(wildcard ../dsp/*.hpp)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILD)/render: render.cpp patches.hpp $(wildcard ../dsp/*.hpp)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: run clean renders
run: $(BUILD)/host_tests
	@mkdir -p out
	./$(BUILD)/host_tests out

# The docs' listening page: the patches and the macro sweeps as WAVs.
renders: $(BUILD)/render
	@mkdir -p out/listen
	./$(BUILD)/render out/listen

clean:
	rm -rf $(BUILD) out
