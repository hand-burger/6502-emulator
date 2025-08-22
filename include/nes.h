#ifndef NES_H
#define NES_H

#include "cpu.h"
#include "ppu.h"
#include "cartridge.h"
#include <memory>

class NES {
public:
    NES();
    ~NES();

    bool loadCartridge(const std::string& romPath);
    void reset();
    void step();
    
    // Get frame buffer for rendering
    const uint8_t* getFrameBuffer() const;
    bool isFrameReady() const;
    void clearFrameReady();
    
    // Controller input
    void setControllerState(uint8_t controller, uint8_t state);

private:
    std::unique_ptr<cpu> m_cpu;
    std::unique_ptr<PPU> m_ppu;
    std::unique_ptr<Cartridge> m_cartridge;
    
    uint8_t m_controller1;
    uint8_t m_controller2;
    uint8_t m_controller1_shift;
    uint8_t m_controller2_shift;
    
    // Internal RAM (2KB, mirrored)
    uint8_t m_ram[0x800];
    
    // Memory mapping
    uint8_t cpuRead(uint16_t address);
    void cpuWrite(uint16_t address, uint8_t value);
    
    // PPU communication
    uint8_t ppuRead(uint16_t address);
    void ppuWrite(uint16_t address, uint8_t value);
    
    // Memory synchronization with CPU
    void updateCPUMemory();
    void syncFromCPUMemory();
    
    friend class cpu; // Allow CPU to access our memory functions
};

#endif // NES_H