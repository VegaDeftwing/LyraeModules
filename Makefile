# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += presets

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

# DSP tests link the real Rack engine; no GUI or audio device is required.
.PHONY: test benchmark
test: build/tests/regression
	LD_LIBRARY_PATH="$(RACK_DIR):$$LD_LIBRARY_PATH" $<

benchmark: build/tests/regression
	LD_LIBRARY_PATH="$(RACK_DIR):$$LD_LIBRARY_PATH" $< --benchmark

build/tests/regression: tests/regression.cpp Makefile $(wildcard tests/legacy/*.hpp) $(wildcard src/*.cpp src/*.hpp)
	mkdir -p build/tests
	$(CXX) -Isrc $(CXXFLAGS) $< -L$(RACK_DIR) -lRack -o $@

build/tests/aladfar: tests/aladfar.cpp $(wildcard src/Aladfar* src/dsp/*.hpp) Makefile
	mkdir -p build/tests
	$(CXX) -iquote src $(CXXFLAGS) $< -L$(RACK_DIR) -lRack -o $@
test: test-aladfar
.PHONY: test-aladfar benchmark-aladfar
test-aladfar: build/tests/aladfar
	LD_LIBRARY_PATH="$(RACK_DIR):$$LD_LIBRARY_PATH" $<
benchmark-aladfar: build/tests/aladfar
	LD_LIBRARY_PATH="$(RACK_DIR):$$LD_LIBRARY_PATH" $< --benchmark

build/tests/aladfar-sanitized: tests/aladfar.cpp $(wildcard src/Aladfar* src/dsp/*.hpp) Makefile
	mkdir -p build/tests
	$(CXX) -iquote src $(filter-out -O3 -funsafe-math-optimizations,$(CXXFLAGS)) -O1 -fsanitize=address,undefined $< -L$(RACK_DIR) -lRack -o $@
.PHONY: test-aladfar-sanitized
test-aladfar-sanitized: build/tests/aladfar-sanitized
	LD_LIBRARY_PATH="$(RACK_DIR):$$LD_LIBRARY_PATH" $<
