# Simple Makefile for testing ether architecture
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
FRAMEWORKS = -framework CoreAudio -framework AudioUnit -framework CoreMIDI -framework Foundation
INCLUDES = -I.

# Source files
SOURCES = \
	src/core/EtherSynth.cpp \
	src/core/PresetManager.cpp \
	src/audio/AudioEngine.cpp \
	src/audio/VoiceManager.cpp \
	src/instruments/InstrumentSlot.cpp \
	src/synthesis/SynthEngine.cpp \
	src/synthesis/SubtractiveEngine.cpp \
	src/synthesis/WavetableEngine.cpp \
	src/synthesis/FMEngine.cpp \
	src/synthesis/GranularEngine.cpp \
	src/sequencer/Timeline.cpp \
	src/sequencer/EuclideanRhythm.cpp \
	src/modulation/ModulationMatrix.cpp \
	src/modulation/AdvancedModulationMatrix.cpp \
	src/effects/EffectsChain.cpp \
	src/hardware/HardwareInterface.cpp \
	src/hardware/MacHardware.cpp \
	src/midi/MIDIManager.cpp \
	EtherSynthBridge.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Targets
all: test_build ether_main

test_build: test_build.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DPLATFORM_MAC $< $(OBJECTS) -o $@ $(FRAMEWORKS)

ether_main: src/main.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DPLATFORM_MAC $< $(OBJECTS) -o $@ $(FRAMEWORKS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DPLATFORM_MAC -c $< -o $@

clean:
	rm -f $(OBJECTS) test_build ether_main

.PHONY: all clean

# Print compilation info
info:
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Frameworks: $(FRAMEWORKS)"
	@echo "Sources: $(words $(SOURCES)) files"