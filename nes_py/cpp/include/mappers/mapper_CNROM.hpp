#ifndef MAPPERCNROM_H
#define MAPPERCNROM_H
#include "mapper.hpp"

class MapperCNROM : public Mapper {

private:
    bool m_oneBank;
    Address m_selectCHR;

public:
    MapperCNROM(Cartridge& cart);
    void writePRG (Address addr, uint8_t value);
    uint8_t readPRG (Address addr);
    const uint8_t* getPagePtr(Address addr);
    uint8_t readCHR (Address addr);
    void writeCHR (Address addr, uint8_t value);

};

#endif // MAPPERCNROM_H
