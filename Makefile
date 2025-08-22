# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++14 -Iinclude $(shell pkg-config --cflags sdl2) -Wall

# Linker flags (link-time only)
LDFLAGS = $(shell pkg-config --libs sdl2)

# Executables
TARGET = emu
NES_TARGET = nes-emu

# Directories
SRCDIR = src
OBJDIR = build/obj
BINDIR = build/bin

# Source files
SRCS = main.cpp cpu.cpp frontend.cpp
NES_SRCS = nes_main.cpp cpu.cpp nes.cpp ppu.cpp cartridge.cpp nes_frontend.cpp

# Object files
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))
NES_OBJS = $(addprefix $(OBJDIR)/,$(NES_SRCS:.cpp=.o))

# VPATH tells make where to find source files
VPATH = $(SRCDIR)

# Default target
all: $(BINDIR)/$(TARGET) $(BINDIR)/$(NES_TARGET)

# Link object files to create the executables
$(BINDIR)/$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(BINDIR)/$(NES_TARGET): $(NES_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(NES_OBJS) $(LDFLAGS)

# Compile source files into object files
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create directories if they don’t exist
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

# Clean up build artifacts
clean:
	rm -rf build

.PHONY: all clean
