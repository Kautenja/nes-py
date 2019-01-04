#ifndef MAPPERCNROM_H
#define MAPPERCNROM_H
#include "mapper.hpp"

class MapperCNROM : public Mapper {

private:
    bool m_oneBank;
    Address m_selectCHR;

public:
    MapperCNROM(Cartridge& cart);
    void writePRG (Address addr, Byte value);
    Byte readPRG (Address addr);
    const Byte* getPagePtr(Address addr);
    Byte readCHR (Address addr);
    void writeCHR (Address addr, Byte value);

};

#endif // MAPPERCNROM_H
