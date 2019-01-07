//  Program:      nes-py
//  File:         mapper_NROM.cpp
//  Description:  An implementation of the NROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_NROM.hpp"
#include "log.hpp"

MapperNROM::MapperNROM(Cartridge &cart) : Mapper(cart) {
    if (cart.getROM().size() == 0x4000) // 1 bank
        m_oneBank = true;
    else // 2 banks
        m_oneBank = false;

    if (cart.getVROM().size() == 0) {
        m_usesCharacterRAM = true;
        m_characterRAM.resize(0x2000);
        LOG(Info) << "Uses character RAM" << std::endl;
    }
    else
        m_usesCharacterRAM = false;
}

NES_Byte MapperNROM::readPRG(NES_Address addr) {
    if (!m_oneBank)
        return m_cartridge.getROM()[addr - 0x8000];
    else //mirrored
        return m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
}

void MapperNROM::writePRG(NES_Address addr, NES_Byte value) {
    LOG(InfoVerbose) << "ROM memory write attempt at " << +addr << " to set " << +value << std::endl;
}

NES_Byte MapperNROM::readCHR(NES_Address addr) {
    if (m_usesCharacterRAM)
        return m_characterRAM[addr];
    else
        return m_cartridge.getVROM()[addr];
}

void MapperNROM::writeCHR(NES_Address addr, NES_Byte value) {
    if (m_usesCharacterRAM)
        m_characterRAM[addr] = value;
    else
        LOG(Info) << "Read-only CHR memory write attempt at " << std::hex << addr << std::endl;
}

const NES_Byte* MapperNROM::getPagePtr(NES_Address addr) {
    if (!m_oneBank)
        return &m_cartridge.getROM()[addr - 0x8000];
    else //mirrored
        return &m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
}
