#ifndef MAPPERNROM_H
#define MAPPERNROM_H
#include "mapper.hpp"

class MapperNROM : public Mapper {

private:
    bool m_oneBank;
    bool m_usesCharacterRAM;
    std::vector<uint8_t> m_characterRAM;

public:
    MapperNROM(Cartridge& cart);
    void writePRG (uint16_t addr, uint8_t value);
    uint8_t readPRG (uint16_t addr);
    const uint8_t* getPagePtr(uint16_t addr);
    uint8_t readCHR (uint16_t addr);
    void writeCHR (uint16_t addr, uint8_t value);

};

#endif // MAPPERNROM_H
