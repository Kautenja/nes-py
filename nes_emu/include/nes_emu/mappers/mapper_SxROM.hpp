//  Program:      nes-py
//  File:         mapper_SxROM.hpp
//  Description:  An implementation of the SxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERSXROM_HPP
#define MAPPERSXROM_HPP

#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

class MapperSxROM : public Mapper {
 private:
    /// the mode for CHR ROM
    int mode_chr;
    /// the mode for PRG ROM
    int mode_prg;
    /// a temporary register
    NES_Byte temp_register;
    /// a write counter
    int write_counter;
    /// the PRG register
    NES_Byte register_prg;
    /// The first CHR register
    NES_Byte register_chr0;
    /// The second CHR register
    NES_Byte register_chr1;
    /// PRG window mapped at $8000-$bfff.
    MapperBank::BankWindow first_prg;
    /// PRG window mapped at $c000-$ffff.
    MapperBank::BankWindow second_prg;
    /// CHR window mapped at PPU $0000-$0fff.
    MapperBank::BankWindow first_chr;
    /// CHR window mapped at PPU $1000-$1fff.
    MapperBank::BankWindow second_chr;
    /// Optional writable CHR RAM.
    MapperBank::CHRMemory chr_memory;

    /// Recalculate PRG windows from the current MMC1 control and PRG registers.
    void calculatePRGWindows();

    /// Recalculate CHR windows from the current MMC1 CHR mode/registers.
    void calculateCHRWindows();

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperSxROM(Cartridge* cart);

    /// Return a copy of this mapper and its current state.
    inline std::unique_ptr<Mapper> clone() const {
        return std::unique_ptr<Mapper>(new MapperSxROM(*this));
    }

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    inline NES_Byte readPRG(NES_Address address) {
        if (address < 0xc000)
            return first_prg.read(cartridge->getROM(), address, 0x8000);
        return second_prg.read(cartridge->getROM(), address, 0xc000);
    }

    /// Return a direct 8 KiB PRG read page for CPU hot paths.
    const NES_Byte* getDirectPRGReadPage(NES_Address page_base);

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG(NES_Address address, NES_Byte value);

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    inline NES_Byte readCHR(NES_Address address) {
        if (chr_memory.usesRAM())
            return chr_memory.read(address);
        if (address < 0x1000)
            return first_chr.read(cartridge->getVROM(), address, 0x0000);
        return second_chr.read(cartridge->getVROM(), address, 0x1000);
    }

    /// Return a direct 1 KiB CHR read page for PPU hot paths.
    const NES_Byte* getDirectCHRReadPage(NES_Address page_base);

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR(NES_Address address, NES_Byte value);

};

}  // namespace NES

#endif  // MAPPERSXROM_HPP
