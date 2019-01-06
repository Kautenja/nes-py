//  Program:      nes-py
//  File:         mapper_SxROM.hpp
//  Description:  An implementation of the SxROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPERSXROM_H
#define MAPPERSXROM_H

#include "mapper.hpp"

class MapperSxROM : public Mapper {

private:
    /// The mirroring callback on the PPU
    std::function<void(void)> m_mirroringCallback;
    /// the mirroring mode on the device
    NameTableMirroring m_mirroing;
    /// whether the cartridge uses character RAM
    bool m_usesCharacterRAM;
    /// the mode for CHR ROM
    int m_modeCHR;
    /// the mode for PRG ROM
    int m_modePRG;
    /// a temporary register
    uint8_t m_tempRegister;
    /// a write counter
    int m_writeCounter;
    /// TODO: what are these variables
    uint8_t m_regPRG;
    uint8_t m_regCHR0;
    uint8_t m_regCHR1;
    const uint8_t* m_firstBankPRG;
    const uint8_t* m_secondBankPRG;
    const uint8_t* m_firstBankCHR;
    const uint8_t* m_secondBankCHR;
    /// The character RAM on the cartridge
    std::vector<uint8_t> m_characterRAM;

    /// TODO: what does this do
    void calculatePRGPointers();

public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    MapperSxROM(Cartridge& cart, std::function<void(void)> mirroring_cb);

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

    /// Return the name table mirroring mode of this mapper.
    NameTableMirroring getNameTableMirroring() { return m_mirroing; };

};

#endif // MAPPERSXROM_H
