#include "mappers/mapper_UxROM.hpp"
#include "log.hpp"

MapperUxROM::MapperUxROM(Cartridge &cart) :
    Mapper(cart, Mapper::UxROM),
    m_selectPRG(0)
{
    if (cart.getVROM().size() == 0)
    {
        m_usesCharacterRAM = true;
        m_characterRAM.resize(0x2000);
        LOG(Info) << "Uses character RAM" << std::endl;
    }
    else
        m_usesCharacterRAM = false;

    m_lastBankPtr = &cart.getROM()[cart.getROM().size() - 0x4000]; //last - 16KB
}

uint8_t MapperUxROM::readPRG(Address addr)
{
    if (addr < 0xc000)
        return m_cartridge.getROM()[((addr - 0x8000) & 0x3fff) | (m_selectPRG << 14)];
    else
        return *(m_lastBankPtr + (addr & 0x3fff));
}

void MapperUxROM::writePRG(Address addr, uint8_t value)
{
    m_selectPRG = value;
}

const uint8_t* MapperUxROM::getPagePtr(Address addr)
{
    if (addr < 0xc000)
        return &m_cartridge.getROM()[((addr - 0x8000) & 0x3fff) | (m_selectPRG << 14)];
    else
        return m_lastBankPtr + (addr & 0x3fff);
}

uint8_t MapperUxROM::readCHR(Address addr)
{
    if (m_usesCharacterRAM)
        return m_characterRAM[addr];
    else
        return m_cartridge.getVROM()[addr];
}

void MapperUxROM::writeCHR(Address addr, uint8_t value)
{
    if (m_usesCharacterRAM)
        m_characterRAM[addr] = value;
    else
        LOG(Info) << "Read-only CHR memory write attempt at " << std::hex << addr << std::endl;
}
