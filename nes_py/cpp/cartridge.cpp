//  Program:      nes-py
//  File:         cartridge.cpp
//  Description:  This class houses the logic and data for an NES cartridge
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "cartridge.hpp"
#include "log.hpp"
#include <fstream>
#include <string>

void Cartridge::loadFromFile(std::string path) {
    // create a stream to load the ROM file
    std::ifstream romFile (path, std::ios_base::binary | std::ios_base::in);
    // create a byte vector for the iNES header
    std::vector<uint8_t> header;
    header.resize(0x10);
    romFile.read(reinterpret_cast<char*>(&header[0]), 0x10);
    // read internal data
    m_nameTableMirroring = header[6] & 0xB;
    m_mapperNumber = ((header[6] >> 4) & 0xf) | (header[7] & 0xf0);
    m_extendedRAM = header[6] & 0x2;
    // read PRG-ROM 16KB banks
    uint8_t banks = header[4];
    m_PRG_ROM.resize(0x4000 * banks);
    romFile.read(reinterpret_cast<char*>(&m_PRG_ROM[0]), 0x4000 * banks);
    // read CHR-ROM 8KB banks
    uint8_t vbanks = header[5];
    if (!vbanks)
        return;
    m_CHR_ROM.resize(0x2000 * vbanks);
    romFile.read(reinterpret_cast<char*>(&m_CHR_ROM[0]), 0x2000 * vbanks);
}
