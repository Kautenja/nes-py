#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <vector>
#include <string>
#include <cstdint>

using Address = std::uint16_t;

class Cartridge
{
public:
    Cartridge();
    bool loadFromFile(std::string path);
    const std::vector<uint8_t>& getROM();
    const std::vector<uint8_t>& getVROM();
    uint8_t getMapper();
    uint8_t getNameTableMirroring();
    bool hasExtendedRAM();
private:
    std::vector<uint8_t> m_PRG_ROM;
    std::vector<uint8_t> m_CHR_ROM;
    uint8_t m_nameTableMirroring;
    uint8_t m_mapperNumber;
    bool m_extendedRAM;
    bool m_chrRAM;
};

#endif // CARTRIDGE_H
