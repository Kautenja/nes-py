//  Program:      nes-py
//  File:         mapper_UxROM.cpp
//  Description:  An implementation of the UxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_UxROM.hpp"
#include "log.hpp"

MapperUxROM::MapperUxROM(Cartridge &cart) :
    Mapper(cart),
    m_selectPRG(0) {
    if (cart.getVROM().size() == 0) {
        m_usesCharacterRAM = true;
        m_characterRAM.resize(0x2000);
        LOG(Info) << "Uses character RAM" << std::endl;
    }
    else
        m_usesCharacterRAM = false;

    m_lastBankPtr = &cart.getROM()[cart.getROM().size() - 0x4000]; //last - 16KB
}

NES_Byte MapperUxROM::readPRG(NES_Address address) {
    if (address < 0xc000)
        return m_cartridge.getROM()[((address - 0x8000) & 0x3fff) | (m_selectPRG << 14)];
    else
        return *(m_lastBankPtr + (address & 0x3fff));
}

const NES_Byte* MapperUxROM::getPagePtr(NES_Address address) {
    if (address < 0xc000)
        return &m_cartridge.getROM()[((address - 0x8000) & 0x3fff) | (m_selectPRG << 14)];
    else
        return m_lastBankPtr + (address & 0x3fff);
}

NES_Byte MapperUxROM::readCHR(NES_Address address) {
    if (m_usesCharacterRAM)
        return m_characterRAM[address];
    else
        return m_cartridge.getVROM()[address];
}

void MapperUxROM::writeCHR(NES_Address address, NES_Byte value) {
    if (m_usesCharacterRAM)
        m_characterRAM[address] = value;
    else
        LOG(Info) << "Read-only CHR memory write attempt at " << std::hex << address << std::endl;
}
