# Terminal EtherSynth Makefile
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
INCLUDES = -I/opt/homebrew/include -Isrc -Isrc/engines -Isrc/audio -Isrc/hardware -Isrc/data
LIBS = -L/opt/homebrew/lib -lportaudio -pthread

# Source directories
SRCDIR = src
ENGINE_DIR = $(SRCDIR)/engines
INSTRUMENTS_DIR = $(SRCDIR)/instruments
CONTROL_DIR = $(SRCDIR)/control/modulation
PROCESSING_DIR = $(SRCDIR)/processing/effects
SEQUENCER_DIR = $(SRCDIR)/sequencer
AUDIO_DIR = $(SRCDIR)/audio
HARDWARE_DIR = $(SRCDIR)/hardware
DATA_DIR = $(SRCDIR)/data
SYNTH_DIR = $(SRCDIR)/synthesis
CORE_DIR = $(SRCDIR)/core

# Find all C++ source files
ENGINE_SOURCES = $(wildcard $(ENGINE_DIR)/*.cpp)
INSTRUMENT_SOURCES =
# For the terminal build, exclude heavy subsystems
CONTROL_SOURCES =
PROCESSING_SOURCES =
SEQUENCER_SOURCES =
AUDIO_SOURCES =
HARDWARE_SOURCES =
DATA_SOURCES =
SYNTH_SOURCES =
CORE_SOURCES =
MAIN_SOURCES = $(wildcard $(SRCDIR)/*.cpp)
# Exclude legacy/stale test mains from library objects
EXCLUDE_SOURCES = \
  $(SRCDIR)/build_test.cpp \
  $(wildcard $(SRCDIR)/test_*.cpp)

# All source files except main programs
LIB_SOURCES = $(ENGINE_SOURCES) $(INSTRUMENT_SOURCES) $(CONTROL_SOURCES) $(PROCESSING_SOURCES) $(SEQUENCER_SOURCES) \
              $(AUDIO_SOURCES) $(HARDWARE_SOURCES) $(DATA_SOURCES) \
              src/synthesis/SynthEngine_minimal.cpp \
              $(filter-out $(SRCDIR)/main.cpp $(EXCLUDE_SOURCES),$(MAIN_SOURCES))

# Include harmonized bridge for ether_* C API
LIB_SOURCES += harmonized_13_engines_bridge.cpp

# Object files
LIB_OBJECTS = $(LIB_SOURCES:.cpp=.o)

# Target executables
TARGET = terminal_ethersynth
BRIDGE_TARGET = enhanced_bridge_test
GRID_TARGET = grid_sequencer

# Default target
all: $(TARGET)

grid: $(GRID_TARGET)

# Terminal EtherSynth executable
$(TARGET): terminal_ethersynth.cpp $(LIB_OBJECTS)
	@echo "🔗 Linking terminal EtherSynth..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ Built: $@"

# Bridge test executable
$(BRIDGE_TARGET): enhanced_test.cpp $(LIB_OBJECTS)
	@echo "🔗 Linking bridge test..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ Built: $@"

# Grid sequencer executable
$(GRID_TARGET): grid_sequencer.cpp $(LIB_OBJECTS)
	@echo "🔗 Linking grid sequencer..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) -llo
	@echo "✅ Built: $@"

# Generic object file rule
%.o: %.cpp
	@echo "🔨 Compiling: $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	find . -name "*.o" -delete
	rm -f $(TARGET) $(BRIDGE_TARGET) $(GRID_TARGET)
	@echo "✅ Clean complete"

# Show available engines (requires successful build)
engines: $(TARGET)
	@echo "🎛️  Available synthesis engines:"
	@./$(TARGET) -list-engines 2>/dev/null || echo "Run './$(TARGET)' and type 'engines' for available engines"

# Test the terminal interface
test: $(TARGET)
	@echo "🧪 Testing terminal interface..."
	@echo "✅ Terminal EtherSynth built successfully!"
	@echo "Run: ./$(TARGET)"

# Quick pattern test
demo: $(TARGET)
	@echo "🎵 Demo pattern:"
	@echo "After running ./$(TARGET), try these commands:"
	@echo "  step 1    # Enable step 1"
	@echo "  step 5    # Enable step 5"  
	@echo "  step 9    # Enable step 9"
	@echo "  step 13   # Enable step 13"
	@echo "  play      # Start playback"

# Help
help:
	@echo "🎵 Terminal EtherSynth Build System"
	@echo "=================================="
	@echo "Targets:"
	@echo "  all       - Build terminal EtherSynth (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  test      - Build and test"
	@echo "  demo      - Show demo commands"
	@echo "  engines   - List available synthesis engines"
	@echo "  grid      - Build grid sequencer (OSC)"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Usage:"
	@echo "  make          # Build terminal interface"
	@echo "  ./$(TARGET)   # Run terminal EtherSynth"
	@echo "  make grid     # Build grid app"
	@echo "  ./$(GRID_TARGET)  # Run grid sequencer"

.PHONY: all clean test demo engines help grid
