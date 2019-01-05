#ifndef MAPPERUXROM_H
#define MAPPERUXROM_H
#include "mapper.hpp"

class MapperUxROM : public Mapper {

private:
    bool m_usesCharacterRAM;
    const uint8_t* m_lastBankPtr;
    Address m_selectPRG;
    std::vector<uint8_t> m_characterRAM;

public:
    MapperUxROM(Cartridge& cart);
    void writePRG (Address addr, uint8_t value);
    uint8_t readPRG (Address addr);
    const uint8_t* getPagePtr(Address addr);
    uint8_t readCHR (Address addr);
    void writeCHR (Address addr, uint8_t value);

};

#endif // MAPPERUXROM_H
