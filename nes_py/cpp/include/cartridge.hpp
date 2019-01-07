//  Program:      nes-py
//  File:         cartridge.hpp
//  Description:  This class houses the logic and data for an NES cartridge
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "common.hpp"
#include <vector>

/// A cartridge holding game ROM and a special hardware mapper emulation
class Cartridge {

private:
    /// the PRG ROM
    std::vector<NES_Byte> m_PRG_ROM;
    /// the CHR ROM
    std::vector<NES_Byte> m_CHR_ROM;
    /// the name table mirroring mode
    NES_Byte m_nameTableMirroring;
    /// the mapper ID number
    NES_Byte m_mapperNumber;
    /// whether this cartridge uses extended RAM
    bool m_extendedRAM;
    /// whether this cartridge has CHR RAM
    /// TODO: delete? was commented in SimpleNES repository
    // bool m_chrRAM;

public:
    /// Initialize a new cartridge
    Cartridge() : m_nameTableMirroring(0), m_mapperNumber(0), m_extendedRAM(false) { };

    /// Load a ROM file into the cartridge and build the corresponding mapper.
    void loadFromFile(std::string path);

    /// Return the ROM data.
    const std::vector<NES_Byte>& getROM() { return m_PRG_ROM; };

    /// Return the VROM data.
    const std::vector<NES_Byte>& getVROM() { return m_CHR_ROM; };

    /// Return the mapper ID number.
    NES_Byte getMapper() { return m_mapperNumber; };

    /// Return the name table mirroring mode.
    NES_Byte getNameTableMirroring() { return m_nameTableMirroring; };

    /// Return a boolean determining whether this cartridge uses extended RAM.
    bool hasExtendedRAM() { return m_extendedRAM; };

};

#endif // CARTRIDGE_H
