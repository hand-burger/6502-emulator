#ifndef PPU_H
#define PPU_H

#include <cstdint>

class PPU {
public:
    PPU();
    ~PPU();
    
    void reset();
    void step(); // Execute one PPU cycle
    
    // PPU register access (through CPU)
    uint8_t readRegister(uint16_t address);
    void writeRegister(uint16_t address, uint8_t value);
    
    // Memory access
    uint8_t readMemory(uint16_t address);
    void writeMemory(uint16_t address, uint8_t value);
    
    // Frame buffer access
    const uint8_t* getFrameBuffer() const { return m_frameBuffer; }
    bool isFrameComplete() const { return m_frameComplete; }
    void clearFrameComplete() { m_frameComplete = false; }
    
    // NMI signal for CPU
    bool getNMI() const { return m_nmi; }
    void clearNMI() { m_nmi = false; }

private:
    // PPU registers
    uint8_t m_ctrl;     // $2000
    uint8_t m_mask;     // $2001
    uint8_t m_status;   // $2002
    uint8_t m_oamAddr;  // $2003
    uint8_t m_scroll;   // $2005
    uint8_t m_addr;     // $2006
    uint8_t m_data;     // $2007
    
    // Internal state
    uint16_t m_vramAddr;
    uint16_t m_tempAddr;
    uint8_t m_fineX;
    bool m_writeToggle;
    
    // Rendering state
    int m_scanline;
    int m_cycle;
    bool m_frameComplete;
    bool m_nmi;
    
    // Memory
    uint8_t m_vram[0x1000];     // 4KB VRAM (nametables)
    uint8_t m_palette[0x20];    // 32 bytes palette RAM
    uint8_t m_oam[0x100];       // 256 bytes OAM (sprite data)
    
    // Frame buffer (256x240 pixels, indexed color)
    uint8_t m_frameBuffer[256 * 240];
    
    // Rendering functions
    void renderPixel();
    void updateScrollRegisters();
};

#endif // PPU_H