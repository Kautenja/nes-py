//  Program:      nes-py
//  File:         mapper_NROM.hpp
//  Description:  An implementation of the NROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERNROM_H
#define MAPPERNROM_H

#include "common.hpp"
#include "mapper.hpp"

class MapperNROM : public Mapper {

private:
    /// whether there are 1 or 2 banks
    bool m_oneBank;
    /// whether this mapper uses character RAM
    bool m_usesCharacterRAM;
    /// the character RAM on the mapper
    std::vector<NES_Byte> m_characterRAM;

public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    MapperNROM(Cartridge& cart);

    /// Read a byte from the PRG RAM.
    ///
    /// @param addr the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    NES_Byte readPRG (NES_Address addr);

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param addr the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG (NES_Address addr, NES_Byte value);

    /// Read a byte from the CHR RAM.
    ///
    /// @param addr the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    NES_Byte readCHR (NES_Address addr);

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param addr the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR (NES_Address addr, NES_Byte value);

    /// Return the page pointer for the given address.
    ///
    /// @param addr the address of the page pointer to get
    /// @return the page pointer at the given address
    ///
    const NES_Byte* getPagePtr(NES_Address addr);

};

#endif // MAPPERNROM_H
