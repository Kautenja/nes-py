//  Program:      nes-py
//  File:         mapper_AxROM.hpp
//  Description:  An implementation of the AxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERAXROM_HPP
#define MAPPERAXROM_HPP

#include "nes_emu/common.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/mapper_bank.hpp"

namespace NES {

class MapperAxROM : public Mapper {
 private:
    /// Switchable 32 KiB PRG window mapped at $8000-$ffff.
    MapperBank::BankWindow selected_prg;
    /// Fixed 8 KiB CHR ROM window for unusual non-CHR-RAM dumps.
    MapperBank::BankWindow chr_rom;
    /// Writable CHR RAM used by standard AxROM cartridges.
    MapperBank::CHRMemory chr_memory;
    /// Whether writes should be resolved through visible PRG data.
    bool bus_conflicts;

 public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    explicit MapperAxROM(Cartridge* cart);

    /// Return a copy of this mapper and its current state.
    inline std::unique_ptr<Mapper> clone() const {
        return std::unique_ptr<Mapper>(new MapperAxROM(*this));
    }

    /// Read a byte from the PRG ROM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG ROM
    ///
    NES_Byte readPRG(NES_Address address);

    /// Write a byte to an address in the PRG ROM area.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG(NES_Address address, NES_Byte value);

    /// Read a byte from CHR memory.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR memory
    ///
    NES_Byte readCHR(NES_Address address);

    /// Write a byte to CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR(NES_Address address, NES_Byte value);

    /// Return true if writes should be resolved against visible PRG data.
    inline bool hasBusConflicts() const { return bus_conflicts; }
};

}  // namespace NES

#endif  // MAPPERAXROM_HPP
