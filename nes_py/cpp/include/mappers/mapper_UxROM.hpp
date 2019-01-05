#ifndef MAPPERUXROM_H
#define MAPPERUXROM_H
#include "mapper.hpp"

class MapperUxROM : public Mapper {

private:
    bool m_usesCharacterRAM;
    const uint8_t* m_lastBankPtr;
    uint16_t m_selectPRG;
    std::vector<uint8_t> m_characterRAM;

public:
    MapperUxROM(Cartridge& cart);
    void writePRG (uint16_t addr, uint8_t value);
    uint8_t readPRG (uint16_t addr);
    const uint8_t* getPagePtr(uint16_t addr);
    uint8_t readCHR (uint16_t addr);
    void writeCHR (uint16_t addr, uint8_t value);

};

#endif // MAPPERUXROM_H
