#ifndef MAPPERNROM_H
#define MAPPERNROM_H
#include "mapper.hpp"

class MapperNROM : public Mapper {

private:
    bool m_oneBank;
    bool m_usesCharacterRAM;
    std::vector<Byte> m_characterRAM;

public:
    MapperNROM(Cartridge& cart);
    void writePRG (Address addr, Byte value);
    Byte readPRG (Address addr);
    const Byte* getPagePtr(Address addr);
    Byte readCHR (Address addr);
    void writeCHR (Address addr, Byte value);

};

#endif // MAPPERNROM_H
