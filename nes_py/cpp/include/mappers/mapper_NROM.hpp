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
    void writePRG (Address addr, uint8_t value);
    uint8_t readPRG (Address addr);
    const uint8_t* getPagePtr(Address addr);
    uint8_t readCHR (Address addr);
    void writeCHR (Address addr, uint8_t value);

};

#endif // MAPPERNROM_H
