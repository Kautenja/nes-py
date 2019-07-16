//  Program:      nes-py
//  File:         cartridge.hpp
//  Description:  This class houses the logic and data for an NES cartridge
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CARTRIDGE_HPP
#define CARTRIDGE_HPP

#include <vector>
#include <string>
#include "common.hpp"

namespace NES {

/// A cartridge holding game ROM and a special hardware mapper emulation
class Cartridge {
 private:
    /// the PRG ROM
    std::vector<NES_Byte> prg_rom;
    /// the CHR ROM
    std::vector<NES_Byte> chr_rom;
    /// the name table mirroring mode
    NES_Byte name_table_mirroring;
    /// the mapper ID number
    NES_Byte mapper_number;
    /// whether this cartridge uses extended RAM
    bool has_extended_ram;

 public:
    /// Initialize a new cartridge
    Cartridge() :
        name_table_mirroring(0),
        mapper_number(0),
        has_extended_ram(false) { }

    /// Return the ROM data.
    const inline std::vector<NES_Byte>& getROM() { return prg_rom; }

    /// Return the VROM data.
    const inline std::vector<NES_Byte>& getVROM() { return chr_rom; }

    /// Return the mapper ID number.
    inline NES_Byte getMapper() { return mapper_number; }

    /// Return the name table mirroring mode.
    inline NES_Byte getNameTableMirroring() { return name_table_mirroring; }

    /// Return a boolean determining whether this cartridge uses extended RAM.
    inline bool hasExtendedRAM() { return has_extended_ram; }

    /// Load a ROM file into the cartridge and build the corresponding mapper.
    void loadFromFile(std::string path);
};

}  // namespace NES

#endif // CARTRIDGE_HPP
