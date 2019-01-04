#ifndef MAPPERUXROM_H
#define MAPPERUXROM_H
#include "mapper.hpp"

class MapperUxROM : public Mapper {

private:
    bool m_usesCharacterRAM;
    const Byte* m_lastBankPtr;
    Address m_selectPRG;
    std::vector<Byte> m_characterRAM;

public:
    MapperUxROM(Cartridge& cart);
    void writePRG (Address addr, Byte value);
    Byte readPRG (Address addr);
    const Byte* getPagePtr(Address addr);
    Byte readCHR (Address addr);
    void writeCHR (Address addr, Byte value);

};

#endif // MAPPERUXROM_H
