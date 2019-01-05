#ifndef MAPPERCNROM_H
#define MAPPERCNROM_H
#include "mapper.hpp"

class MapperCNROM : public Mapper {

private:

    bool m_oneBank;

    uint16_t m_selectCHR;

public:

    MapperCNROM(Cartridge& cart) :
        Mapper(cart, Mapper::CNROM),
        m_selectCHR(0) { m_oneBank = cart.getROM().size() == 0x4000; };

    void writePRG (uint16_t addr, uint8_t value) { m_selectCHR = value & 0x3; };

    uint8_t readPRG (uint16_t addr);

    const uint8_t* getPagePtr(uint16_t addr);

    uint8_t readCHR (uint16_t addr) { return m_cartridge.getVROM()[addr | (m_selectCHR << 13)]; };

    void writeCHR (uint16_t addr, uint8_t value);

};

#endif // MAPPERCNROM_H
