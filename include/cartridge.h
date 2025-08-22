#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <vector>
#include <string>
#include <cstdint>

struct INESHeader {
    char signature[4];      // "NES\x1A"
    uint8_t prgRomBanks;    // Number of 16KB PRG ROM banks
    uint8_t chrRomBanks;    // Number of 8KB CHR ROM banks
    uint8_t flags6;         // Control byte 1
    uint8_t flags7;         // Control byte 2
    uint8_t prgRamBanks;    // Number of 8KB PRG RAM banks
    uint8_t flags9;         // TV system
    uint8_t flags10;        // TV system, PRG RAM presence
    uint8_t padding[5];     // Unused padding
};

class Cartridge {
public:
    Cartridge();
    ~Cartridge();
    
    bool loadFromFile(const std::string& filename);
    void reset();
    
    // CPU memory access
    uint8_t cpuRead(uint16_t address);
    void cpuWrite(uint16_t address, uint8_t value);
    
    // PPU memory access
    uint8_t ppuRead(uint16_t address);
    void ppuWrite(uint16_t address, uint8_t value);
    
    bool isValid() const { return m_valid; }
    uint8_t getMapper() const { return m_mapper; }

private:
    bool m_valid;
    uint8_t m_mapper;
    uint8_t m_prgBanks;
    uint8_t m_chrBanks;
    
    std::vector<uint8_t> m_prgRom;
    std::vector<uint8_t> m_chrRom;
    std::vector<uint8_t> m_prgRam;
    
    // Mirroring
    enum Mirroring {
        HORIZONTAL,
        VERTICAL,
        FOUR_SCREEN
    } m_mirroring;
    
    bool parseHeader(const INESHeader& header);
};

#endif // CARTRIDGE_H