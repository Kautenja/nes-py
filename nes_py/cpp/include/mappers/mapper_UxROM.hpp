//  Program:      nes-py
//  File:         mapper_UxROM.hpp
//  Description:  An implementation of the UxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERUXROM_H
#define MAPPERUXROM_H

#include "mapper.hpp"

class MapperUxROM : public Mapper {

private:
    /// whether the cartridge use character RAM
    bool m_usesCharacterRAM;
    /// the pointer to the last bank
    const uint8_t* m_lastBankPtr;
    /// TODO: what is this?
    uint16_t m_selectPRG;
    /// The character RAM on the mapper
    std::vector<uint8_t> m_characterRAM;

public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    MapperUxROM(Cartridge& cart);

    /// Read a byte from the PRG RAM.
    ///
    /// @param addr the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    uint8_t readPRG (uint16_t addr);

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param addr the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG (uint16_t addr, uint8_t value);

    /// Read a byte from the CHR RAM.
    ///
    /// @param addr the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    uint8_t readCHR (uint16_t addr);

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param addr the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR (uint16_t addr, uint8_t value);

    /// Return the page pointer for the given address.
    ///
    /// @param addr the address of the page pointer to get
    /// @return the page pointer at the given address
    ///
    const uint8_t* getPagePtr(uint16_t addr);

};

#endif // MAPPERUXROM_H
