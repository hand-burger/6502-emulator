# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Iinclude -I/opt/homebrew/include/SDL2 -Wall

# Linker flags (link-time only)
LDFLAGS = -L/opt/homebrew/lib -lSDL2

# Executable name
TARGET = emu

# Directories
SRCDIR = src
OBJDIR = build/obj
BINDIR = build/bin

# Source files
SRCS = main.cpp cpu.cpp frontend.cpp

# Object files
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))

# VPATH tells make where to find source files
VPATH = $(SRCDIR)

# Default target
all: $(BINDIR)/$(TARGET)

# Link object files to create the executable
$(BINDIR)/$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files into object files
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create directories if they don’t exist
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

# Build and run the CPU unit tests (no SDL required)
TESTBIN = $(BINDIR)/cpu_tests

test: $(TESTBIN)
	./$(TESTBIN)

$(TESTBIN): tests/cpu_tests.cpp $(OBJDIR)/cpu.o | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ tests/cpu_tests.cpp $(OBJDIR)/cpu.o

# Clean up build artifacts
clean:
	rm -rf build

.PHONY: all clean test
