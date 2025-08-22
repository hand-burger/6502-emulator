#include "nes.h"
#include <iostream>
#include <cstring>

NES::NES() 
    : m_controller1(0), m_controller2(0)
    , m_controller1_shift(0), m_controller2_shift(0)
{
    m_cpu = std::make_unique<cpu>();
    m_ppu = std::make_unique<PPU>();
    m_cartridge = std::make_unique<Cartridge>();
    
    // Initialize RAM
    std::memset(m_ram, 0, sizeof(m_ram));
}

NES::~NES() = default;

bool NES::loadCartridge(const std::string& romPath) {
    return m_cartridge->loadFromFile(romPath);
}

void NES::reset() {
    m_cpu->reset();
    m_ppu->reset();
    m_cartridge->reset();
    
    m_controller1 = m_controller2 = 0;
    m_controller1_shift = m_controller2_shift = 0;
}

void NES::step() {
    // For now, copy NES memory to CPU memory array for each access
    // This is not efficient but allows us to use the existing CPU
    updateCPUMemory();
    
    // PPU runs 3 times faster than CPU
    m_ppu->step();
    m_ppu->step();
    m_ppu->step();
    
    // Check for NMI from PPU
    if (m_ppu->getNMI()) {
        m_ppu->clearNMI();
        // TODO: Trigger NMI in CPU
    }
    
    // Execute one CPU instruction
    m_cpu->execute();
    
    // Copy any changes back from CPU memory
    syncFromCPUMemory();
}

const uint8_t* NES::getFrameBuffer() const {
    return m_ppu->getFrameBuffer();
}

bool NES::isFrameReady() const {
    return m_ppu->isFrameComplete();
}

void NES::clearFrameReady() {
    m_ppu->clearFrameComplete();
}

void NES::setControllerState(uint8_t controller, uint8_t state) {
    if (controller == 0) {
        m_controller1 = state;
    } else if (controller == 1) {
        m_controller2 = state;
    }
}

uint8_t NES::cpuRead(uint16_t address) {
    if (address < 0x2000) {
        // Internal RAM (2KB, mirrored every 0x800 bytes)
        return m_ram[address & 0x7FF];
    }
    else if (address < 0x4000) {
        // PPU registers (8 bytes, mirrored)
        return m_ppu->readRegister(0x2000 + (address & 0x7));
    }
    else if (address == 0x4016) {
        // Controller 1
        uint8_t result = (m_controller1_shift & 0x80) ? 1 : 0;
        m_controller1_shift <<= 1;
        return result;
    }
    else if (address == 0x4017) {
        // Controller 2
        uint8_t result = (m_controller2_shift & 0x80) ? 1 : 0;
        m_controller2_shift <<= 1;
        return result;
    }
    else if (address >= 0x8000) {
        // Cartridge ROM
        return m_cartridge->cpuRead(address);
    }
    
    return 0; // Open bus
}

void NES::cpuWrite(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        // Internal RAM (2KB, mirrored every 0x800 bytes)
        m_ram[address & 0x7FF] = value;
    }
    else if (address < 0x4000) {
        // PPU registers (8 bytes, mirrored)
        m_ppu->writeRegister(0x2000 + (address & 0x7), value);
    }
    else if (address == 0x4016) {
        // Controller strobe
        if (value & 1) {
            m_controller1_shift = m_controller1;
            m_controller2_shift = m_controller2;
        }
    }
    else if (address >= 0x8000) {
        // Cartridge (some mappers support writes)
        m_cartridge->cpuWrite(address, value);
    }
}

uint8_t NES::ppuRead(uint16_t address) {
    return m_cartridge->ppuRead(address);
}

void NES::ppuWrite(uint16_t address, uint8_t value) {
    m_cartridge->ppuWrite(address, value);
}

void NES::updateCPUMemory() {
    // Copy RAM to CPU memory
    for (int i = 0; i < 0x2000; i += 0x800) {
        std::memcpy(&m_cpu->memory[i], m_ram, 0x800);
    }
    
    // Copy cartridge ROM to CPU memory
    if (m_cartridge->isValid()) {
        for (uint16_t addr = 0x8000; addr < 0x10000; ++addr) {
            m_cpu->memory[addr] = m_cartridge->cpuRead(addr);
        }
    }
}

void NES::syncFromCPUMemory() {
    // Copy RAM back from CPU memory (only the first 0x800 bytes matter)
    std::memcpy(m_ram, &m_cpu->memory[0], 0x800);
}