#include "cartridge.h"
#include <fstream>
#include <iostream>

Cartridge::Cartridge() 
    : m_valid(false), m_mapper(0), m_prgBanks(0), m_chrBanks(0)
    , m_mirroring(HORIZONTAL)
{
}

Cartridge::~Cartridge() = default;

bool Cartridge::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open ROM file: " << filename << std::endl;
        return false;
    }
    
    INESHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (!parseHeader(header)) {
        std::cerr << "Invalid ROM header" << std::endl;
        return false;
    }
    
    // Skip trainer if present
    if (header.flags6 & 0x04) {
        file.seekg(512, std::ios::cur);
    }
    
    // Read PRG ROM
    size_t prgSize = m_prgBanks * 16384; // 16KB banks
    m_prgRom.resize(prgSize);
    file.read(reinterpret_cast<char*>(m_prgRom.data()), prgSize);
    
    // Read CHR ROM
    if (m_chrBanks > 0) {
        size_t chrSize = m_chrBanks * 8192; // 8KB banks
        m_chrRom.resize(chrSize);
        file.read(reinterpret_cast<char*>(m_chrRom.data()), chrSize);
    } else {
        // CHR RAM
        m_chrRom.resize(8192);
    }
    
    // Initialize PRG RAM
    m_prgRam.resize(8192); // 8KB PRG RAM
    
    m_valid = true;
    std::cout << "Loaded ROM: " << filename << std::endl;
    std::cout << "PRG Banks: " << (int)m_prgBanks << ", CHR Banks: " << (int)m_chrBanks << std::endl;
    std::cout << "Mapper: " << (int)m_mapper << std::endl;
    
    return true;
}

bool Cartridge::parseHeader(const INESHeader& header) {
    // Check signature
    if (header.signature[0] != 'N' || header.signature[1] != 'E' || 
        header.signature[2] != 'S' || header.signature[3] != 0x1A) {
        return false;
    }
    
    m_prgBanks = header.prgRomBanks;
    m_chrBanks = header.chrRomBanks;
    
    // Extract mapper number
    m_mapper = (header.flags6 >> 4) | (header.flags7 & 0xF0);
    
    // Extract mirroring
    if (header.flags6 & 0x08) {
        m_mirroring = FOUR_SCREEN;
    } else if (header.flags6 & 0x01) {
        m_mirroring = VERTICAL;
    } else {
        m_mirroring = HORIZONTAL;
    }
    
    return true;
}

void Cartridge::reset() {
    // Reset any mapper state if needed
}

uint8_t Cartridge::cpuRead(uint16_t address) {
    if (!m_valid) return 0;
    
    if (address >= 0x8000) {
        // PRG ROM area
        if (m_prgBanks == 1) {
            // 16KB ROM mirrored
            return m_prgRom[(address - 0x8000) & 0x3FFF];
        } else {
            // 32KB ROM or more
            return m_prgRom[address - 0x8000];
        }
    }
    else if (address >= 0x6000) {
        // PRG RAM area
        return m_prgRam[address - 0x6000];
    }
    
    return 0;
}

void Cartridge::cpuWrite(uint16_t address, uint8_t value) {
    if (!m_valid) return;
    
    if (address >= 0x6000 && address < 0x8000) {
        // PRG RAM area
        m_prgRam[address - 0x6000] = value;
    }
    // ROM areas are read-only for mapper 0
}

uint8_t Cartridge::ppuRead(uint16_t address) {
    if (!m_valid) return 0;
    
    if (address < 0x2000) {
        // Pattern tables
        return m_chrRom[address];
    }
    
    return 0;
}

void Cartridge::ppuWrite(uint16_t address, uint8_t value) {
    if (!m_valid) return;
    
    if (address < 0x2000 && m_chrBanks == 0) {
        // CHR RAM (only if no CHR ROM)
        m_chrRom[address] = value;
    }
}