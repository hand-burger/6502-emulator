#include "ppu.h"
#include <cstring>

PPU::PPU() 
    : m_ctrl(0), m_mask(0), m_status(0), m_oamAddr(0)
    , m_scroll(0), m_addr(0), m_data(0)
    , m_vramAddr(0), m_tempAddr(0), m_fineX(0), m_writeToggle(false)
    , m_scanline(0), m_cycle(0), m_frameComplete(false), m_nmi(false)
{
    std::memset(m_vram, 0, sizeof(m_vram));
    std::memset(m_palette, 0, sizeof(m_palette));
    std::memset(m_oam, 0, sizeof(m_oam));
    std::memset(m_frameBuffer, 0, sizeof(m_frameBuffer));
}

PPU::~PPU() = default;

void PPU::reset() {
    m_ctrl = m_mask = m_status = m_oamAddr = 0;
    m_scroll = m_addr = m_data = 0;
    m_vramAddr = m_tempAddr = m_fineX = 0;
    m_writeToggle = false;
    m_scanline = 0;
    m_cycle = 0;
    m_frameComplete = false;
    m_nmi = false;
    
    std::memset(m_frameBuffer, 0, sizeof(m_frameBuffer));
}

void PPU::step() {
    // Simplified PPU timing - just cycle through scanlines
    m_cycle++;
    
    if (m_cycle > 340) {
        m_cycle = 0;
        m_scanline++;
        
        if (m_scanline == 241) {
            // VBlank start
            m_status |= 0x80; // Set VBlank flag
            if (m_ctrl & 0x80) {
                m_nmi = true; // Generate NMI if enabled
            }
            m_frameComplete = true;
        }
        else if (m_scanline >= 262) {
            // End of frame
            m_scanline = 0;
            m_status &= ~0x80; // Clear VBlank flag
        }
    }
    
    // Render visible scanlines
    if (m_scanline < 240 && m_cycle < 256) {
        renderPixel();
    }
}

void PPU::renderPixel() {
    // Very basic rendering - just fill with a test pattern for now
    int x = m_cycle;
    int y = m_scanline;
    
    if (x >= 0 && x < 256 && y >= 0 && y < 240) {
        // Create a simple test pattern
        uint8_t color = (x / 32 + y / 32) % 4;
        m_frameBuffer[y * 256 + x] = color;
    }
}

uint8_t PPU::readRegister(uint16_t address) {
    uint8_t value = 0;
    
    switch (address & 0x7) {
        case 0x2: // PPUSTATUS
            value = m_status;
            m_status &= ~0x80; // Clear VBlank flag on read
            m_writeToggle = false; // Reset write toggle
            break;
        case 0x4: // OAMDATA
            value = m_oam[m_oamAddr];
            break;
        case 0x7: // PPUDATA
            value = m_data;
            m_data = readMemory(m_vramAddr);
            m_vramAddr += (m_ctrl & 0x04) ? 32 : 1; // Increment by 1 or 32
            break;
        default:
            // Other registers are write-only or not implemented
            break;
    }
    
    return value;
}

void PPU::writeRegister(uint16_t address, uint8_t value) {
    switch (address & 0x7) {
        case 0x0: // PPUCTRL
            m_ctrl = value;
            m_tempAddr = (m_tempAddr & 0x73FF) | ((value & 0x03) << 10);
            break;
        case 0x1: // PPUMASK
            m_mask = value;
            break;
        case 0x3: // OAMADDR
            m_oamAddr = value;
            break;
        case 0x4: // OAMDATA
            m_oam[m_oamAddr++] = value;
            break;
        case 0x5: // PPUSCROLL
            if (!m_writeToggle) {
                m_tempAddr = (m_tempAddr & 0x7FE0) | (value >> 3);
                m_fineX = value & 0x07;
            } else {
                m_tempAddr = (m_tempAddr & 0x0C1F) | ((value & 0x07) << 12) | ((value & 0xF8) << 2);
            }
            m_writeToggle = !m_writeToggle;
            break;
        case 0x6: // PPUADDR
            if (!m_writeToggle) {
                m_tempAddr = (m_tempAddr & 0x00FF) | ((value & 0x3F) << 8);
            } else {
                m_tempAddr = (m_tempAddr & 0x7F00) | value;
                m_vramAddr = m_tempAddr;
            }
            m_writeToggle = !m_writeToggle;
            break;
        case 0x7: // PPUDATA
            writeMemory(m_vramAddr, value);
            m_vramAddr += (m_ctrl & 0x04) ? 32 : 1; // Increment by 1 or 32
            break;
    }
}

uint8_t PPU::readMemory(uint16_t address) {
    address &= 0x3FFF; // PPU addresses are 14-bit
    
    if (address < 0x2000) {
        // Pattern tables - handled by cartridge
        return 0; // TODO: Read from cartridge
    }
    else if (address < 0x3F00) {
        // Nametables
        return m_vram[address & 0x0FFF];
    }
    else {
        // Palette RAM
        address &= 0x1F;
        if ((address & 0x13) == 0x10) address &= ~0x10; // Mirror background colors
        return m_palette[address];
    }
}

void PPU::writeMemory(uint16_t address, uint8_t value) {
    address &= 0x3FFF; // PPU addresses are 14-bit
    
    if (address < 0x2000) {
        // Pattern tables - some cartridges have CHR RAM
        // TODO: Write to cartridge
    }
    else if (address < 0x3F00) {
        // Nametables
        m_vram[address & 0x0FFF] = value;
    }
    else {
        // Palette RAM
        address &= 0x1F;
        if ((address & 0x13) == 0x10) address &= ~0x10; // Mirror background colors
        m_palette[address] = value;
    }
}

void PPU::updateScrollRegisters() {
    // TODO: Implement proper scroll register updates
}