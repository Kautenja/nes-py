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
    NES_Byte m_tempRegister;
    /// a write counter
    int m_writeCounter;
    /// TODO: what are these variables
    NES_Byte m_regPRG;
    NES_Byte m_regCHR0;
    NES_Byte m_regCHR1;
    const NES_Byte* m_firstBankPRG;
    const NES_Byte* m_secondBankPRG;
    const NES_Byte* m_firstBankCHR;
    const NES_Byte* m_secondBankCHR;
    /// The character RAM on the cartridge
    std::vector<NES_Byte> m_characterRAM;

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

    /// Return the name table mirroring mode of this mapper.
    NameTableMirroring getNameTableMirroring() { return m_mirroing; };

};

#endif // MAPPERSXROM_HPP
