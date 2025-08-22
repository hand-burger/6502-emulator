#!/usr/bin/env python3
"""
Create a minimal test NES ROM for testing the emulator
"""
import struct

def create_test_rom():
    # iNES header
    header = bytearray(16)
    header[0:4] = b'NES\x1A'  # Signature
    header[4] = 2             # PRG ROM banks (32KB)
    header[5] = 1             # CHR ROM banks (8KB) 
    header[6] = 0x00          # Flags 6 (Mapper 0, horizontal mirroring)
    header[7] = 0x00          # Flags 7 (Mapper 0)
    header[8] = 0             # PRG RAM size
    header[9] = 0             # TV system
    header[10] = 0            # TV system, PRG RAM
    # Padding (rest is zeros)
    
    # PRG ROM (32KB) - Simple program that writes to PPU
    prg_rom = bytearray(32768)
    
    # Simple 6502 program starting at $8000
    program = [
        0xA2, 0x00,       # LDX #$00
        0x86, 0x00,       # STX $00     ; Clear zero page
        0xA9, 0x3F,       # LDA #$3F    ; Palette address high byte
        0x8D, 0x06, 0x20, # STA $2006   ; PPU address high
        0xA9, 0x00,       # LDA #$00    ; Palette address low byte  
        0x8D, 0x06, 0x20, # STA $2006   ; PPU address low
        0xA9, 0x0F,       # LDA #$0F    ; White color
        0x8D, 0x07, 0x20, # STA $2007   ; Write to PPU data
        0x4C, 0x00, 0x80  # JMP $8000   ; Loop forever
    ]
    
    # Copy program to PRG ROM
    for i, byte in enumerate(program):
        prg_rom[i] = byte
    
    # Set reset vector to $8000
    prg_rom[0x7FFC - 0x8000] = 0x00  # Low byte of $8000
    prg_rom[0x7FFD - 0x8000] = 0x80  # High byte of $8000
    
    # CHR ROM (8KB) - Simple pattern data
    chr_rom = bytearray(8192)
    
    # Create a simple checkerboard pattern
    for i in range(0, 256, 16):  # For each tile
        for row in range(8):
            pattern = 0xAA if row % 2 == 0 else 0x55
            chr_rom[i + row] = pattern
            chr_rom[i + row + 8] = pattern
    
    return header + prg_rom + chr_rom

if __name__ == "__main__":
    rom_data = create_test_rom()
    with open("test.nes", "wb") as f:
        f.write(rom_data)
    print(f"Created test.nes ({len(rom_data)} bytes)")