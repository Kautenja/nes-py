//  Program:      nes-py
//  File:         mapper_NROM.hpp
//  Description:  An implementation of the NROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERNROM_HPP
#define MAPPERNROM_HPP

#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

class MapperNROM : public Mapper {
 private:
    /// PRG window mapped at $8000-$bfff.
    MapperBank::BankWindow first_prg;
    /// PRG window mapped at $c000-$ffff.
    MapperBank::BankWindow second_prg;
    /// CHR ROM window mapped at PPU $0000-$1fff.
    MapperBank::BankWindow chr_rom;
    /// Optional writable CHR RAM.
    MapperBank::CHRMemory chr_memory;

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperNROM(Cartridge* cart);

    /// Return a copy of this mapper and its current state.
    inline std::unique_ptr<Mapper> clone() const {
        return std::unique_ptr<Mapper>(new MapperNROM(*this));
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
        return chr_rom.read(cartridge->getVROM(), address, 0x0000);
    }

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR(NES_Address address, NES_Byte value);

    /// NROM has a stable CHR window; CHR-RAM writes are tracked by PictureBus.
    inline bool allowsSpriteRowPrefetch() const { return true; }
};

}  // namespace NES

#endif // MAPPERNROM_HPP
