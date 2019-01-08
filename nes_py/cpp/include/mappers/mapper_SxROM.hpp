//  Program:      nes-py
//  File:         mapper_SxROM.hpp
//  Description:  An implementation of the SxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERSXROM_HPP
#define MAPPERSXROM_HPP

#include "common.hpp"
#include "mapper.hpp"

class MapperSxROM : public Mapper {

private:
    /// The mirroring callback on the PPU
    std::function<void(void)> mirroring_callback;
    /// the mirroring mode on the device
    NameTableMirroring mirroing;
    /// whether the cartridge uses character RAM
    bool has_character_ram;
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
    /// The first PRG bank
    const NES_Byte* first_bank_prg;
    /// The second PRG bank
    const NES_Byte* second_bank_prg;
    /// The first CHR bank
    const NES_Byte* first_bank_chr;
    /// The second CHR bank
    const NES_Byte* second_bank_chr;
    /// The character RAM on the cartridge
    std::vector<NES_Byte> character_ram;

    /// TODO: what does this do
    void calculatePRGPointers();

public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    /// @param mirroring_cb the callback to change mirroring modes on the PPU
    ///
    MapperSxROM(Cartridge& cart, std::function<void(void)> mirroring_cb);

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    NES_Byte readPRG (NES_Address address);

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG (NES_Address address, NES_Byte value);

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    NES_Byte readCHR (NES_Address address);

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR (NES_Address address, NES_Byte value);

    /// Return the page pointer for the given address.
    ///
    /// @param address the address of the page pointer to get
    /// @return the page pointer at the given address
    ///
    const NES_Byte* getPagePtr(NES_Address address);

    /// Return the name table mirroring mode of this mapper.
    inline NameTableMirroring getNameTableMirroring() { return mirroing; };

};

#endif // MAPPERSXROM_HPP
