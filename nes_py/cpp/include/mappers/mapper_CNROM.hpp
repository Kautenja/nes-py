#ifndef MAPPERCNROM_H
#define MAPPERCNROM_H
#include "mapper.hpp"

class MapperCNROM : public Mapper {

private:

    bool m_oneBank;

    Address m_selectCHR;

public:

    MapperCNROM(Cartridge& cart) :
        Mapper(cart, Mapper::CNROM),
        m_selectCHR(0) { m_oneBank = cart.getROM().size() == 0x4000; };

    void writePRG (Address addr, uint8_t value) { m_selectCHR = value & 0x3; };

    uint8_t readPRG (Address addr);

    const uint8_t* getPagePtr(Address addr);

    uint8_t readCHR (Address addr) { return m_cartridge.getVROM()[addr | (m_selectCHR << 13)]; };

    void writeCHR (Address addr, uint8_t value);

};

#endif // MAPPERCNROM_H
