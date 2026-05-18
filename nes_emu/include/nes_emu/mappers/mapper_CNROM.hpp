//  Program:      nes-py
//  File:         mapper_CNROM.hpp
//  Description:  An implementation of the CNROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERCNROM_HPP
#define MAPPERCNROM_HPP

#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

class MapperCNROM : public Mapper {
 private:
    /// PRG window mapped at $8000-$bfff.
    MapperBank::BankWindow first_prg;
    /// PRG window mapped at $c000-$ffff.
    MapperBank::BankWindow second_prg;
    /// Switchable 8KB CHR ROM window.
    MapperBank::BankWindow selected_chr;

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperCNROM(Cartridge* cart) :
        Mapper(cart) {
        first_prg.selectFirst(cart->getROM().size(), 0x4000);
        second_prg.selectFinal(cart->getROM().size(), 0x4000);
        selected_chr.selectFirst(cart->getVROM().size(), 0x2000);
    }

    /// Return a copy of this mapper and its current state.
    inline std::unique_ptr<Mapper> clone() const {
        return std::unique_ptr<Mapper>(new MapperCNROM(*this));
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
    inline const NES_Byte* getDirectPRGReadPage(NES_Address page_base) {
        if (page_base < 0xc000) {
            return first_prg.readPointer(
                cartridge->getROM(),
                page_base,
                0x8000,
                0x2000
            );
        }
        return second_prg.readPointer(
            cartridge->getROM(),
            page_base,
            0xc000,
            0x2000
        );
    }

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writePRG(NES_Address address, NES_Byte value) {
        (void) address;
        // This implementation models no bus conflicts; MainBus resolves
        // conflicts first for mappers that opt into hasBusConflicts().
        selected_chr.selectBank(cartridge->getVROM().size(), 0x2000, value);
    }

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    inline NES_Byte readCHR(NES_Address address) {
        return selected_chr.read(cartridge->getVROM(), address, 0x0000);
    }

    /// Return a direct 1 KiB CHR read page for PPU hot paths.
    inline const NES_Byte* getDirectCHRReadPage(NES_Address page_base) {
        return selected_chr.readPointer(
            cartridge->getVROM(),
            page_base,
            0x0000,
            0x0400
        );
    }

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR(NES_Address address, NES_Byte value);
};

}  // namespace NES

#endif // MAPPERCNROM_HPP
