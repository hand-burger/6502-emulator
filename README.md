# 6502 Emulator & NES Emulator

This repository now contains both a simple 6502 emulator and a full NES (Nintendo Entertainment System) emulator built on top of the 6502 CPU core.

## Building

Install dependencies:
```bash
sudo apt install libsdl2-dev
```

Build both emulators:
```bash
make
```

This creates two executables:
- `build/bin/emu` - Original 6502 emulator with snake game
- `build/bin/nes-emu` - NES emulator

## 6502 Emulator (`emu`)

The original 6502 emulator runs a hardcoded snake game. 

**Controls:**
- WASD: Snake movement
- Escape: Quit

**Usage:**
```bash
./build/bin/emu
```

## NES Emulator (`nes-emu`)

A basic but functional NES emulator that supports:

- 6502 CPU with NES-specific memory mapping
- PPU (Picture Processing Unit) with basic rendering
- Cartridge loading (.nes format, iNES header)
- NES controller input
- 256x240 resolution display with NES color palette

**Controls:**
- Arrow keys: D-Pad
- Z: A button
- X: B button  
- Tab: Select
- Enter: Start
- Escape: Quit

**Usage:**
```bash
# Run with a ROM file
./build/bin/nes-emu game.nes

# Run without ROM (displays test pattern)
./build/bin/nes-emu
```

## NES Emulator Features

### Supported
- ✅ 6502 CPU core (reused from original emulator)
- ✅ Basic PPU with VBlank timing
- ✅ iNES ROM loading (Mapper 0)
- ✅ Memory mapping ($0000-$1FFF RAM, $2000-$3FFF PPU, $8000-$FFFF ROM)
- ✅ Controller input
- ✅ SDL2-based frontend with proper NES resolution
- ✅ Test pattern rendering when no ROM is loaded

### Planned/TODO
- ⏳ Background tile rendering
- ⏳ Sprite rendering
- ⏳ Scrolling support
- ⏳ Audio/APU support
- ⏳ Additional mappers
- ⏳ Save state support

## Technical Details

The NES emulator is built around these key components:

### CPU
- Reuses the existing 6502 CPU implementation
- Memory access is intercepted for NES-specific mapping
- Supports all original 6502 instructions

### PPU (Picture Processing Unit)
- Handles graphics rendering
- Implements basic VBlank timing
- Manages video memory and palette
- Currently renders a test pattern

### Cartridge
- Loads iNES format ROM files
- Supports Mapper 0 (NROM)
- Handles PRG ROM and CHR ROM data
- Proper memory banking

### Memory Mapping
- $0000-$07FF: 2KB internal RAM (mirrored)
- $2000-$2007: PPU registers (mirrored)
- $4016-$4017: Controller ports
- $8000-$FFFF: Cartridge ROM space

## Creating Test ROMs

A Python script is included to create simple test ROMs:

```python
python3 /tmp/create_test_rom.py
```

This creates `test.nes` with a basic program that writes to the PPU palette.

## Architecture

The emulator maintains clean separation between components:

```
NES
├── CPU (6502 core)
├── PPU (graphics)
├── Cartridge (ROM/mapper)
└── Frontend (SDL2 display/input)
```

Each component communicates through well-defined interfaces, making the codebase modular and extensible.