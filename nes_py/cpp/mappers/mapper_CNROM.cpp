#include "mappers/mapper_CNROM.hpp"
#include "log.hpp"

MapperCNROM::MapperCNROM(Cartridge &cart) :
    Mapper(cart, Mapper::CNROM),
    m_selectCHR(0)
{
    if (cart.getROM().size() == 0x4000) //1 bank
    {
        m_oneBank = true;
    }
    else //2 banks
    {
        m_oneBank = false;
    }
}

uint8_t MapperCNROM::readPRG(Address addr)
{
    if (!m_oneBank)
        return m_cartridge.getROM()[addr - 0x8000];
    else //mirrored
        return m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
}

void MapperCNROM::writePRG(Address addr, uint8_t value)
{
    m_selectCHR = value & 0x3;
}

const uint8_t* MapperCNROM::getPagePtr(Address addr)
{
    if (!m_oneBank)
        return &m_cartridge.getROM()[addr - 0x8000];
    else //mirrored
        return &m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
}

uint8_t MapperCNROM::readCHR(Address addr)
{
    return m_cartridge.getVROM()[addr | (m_selectCHR << 13)];
}

void MapperCNROM::writeCHR(Address addr, uint8_t value)
{
    LOG(Info) << "Read-only CHR memory write attempt at " << std::hex << addr << std::endl;
}
