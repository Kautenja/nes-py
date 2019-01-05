#include "mappers/mapper_CNROM.hpp"
#include "log.hpp"

uint8_t MapperCNROM::readPRG(uint16_t addr) {
    if (!m_oneBank)
        return m_cartridge.getROM()[addr - 0x8000];
    // mirrored
    else
        return m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
}

const uint8_t* MapperCNROM::getPagePtr(uint16_t addr) {
    if (!m_oneBank)
        return &m_cartridge.getROM()[addr - 0x8000];
    // mirrored
    else
        return &m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
}

void MapperCNROM::writeCHR(uint16_t addr, uint8_t value) {
    LOG(Info) << "Read-only CHR memory write attempt at " << std::hex << addr << std::endl;
}
