//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  This class provides an abstraction of an NES cartridge mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPER_H
#define MAPPER_H

#include "cartridge.hpp"
#include <memory>
#include <functional>

/// Mirroring modes supported by the NES
enum NameTableMirroring {
    Horizontal  = 0,
    Vertical    = 1,
    FourScreen  = 8,
    OneScreenLower,
    OneScreenHigher,
};

/// An abstraction of a general hardware mapper for different NES cartridges
class Mapper {
public:
    /// an enumeration of mapper IDs
    enum Type {
        NROM  = 0,
        SxROM = 1,
        UxROM = 2,
        CNROM = 3,
    };

    /// Create a new mapper with a cartridge and given type.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    Mapper(Cartridge& cart) : m_cartridge(cart) { };

    /// Create a mapper based on given type, a game cartridge
    static Mapper* create (Cartridge& cart, std::function<void(void)> mirroring_cb);

    /// Read a byte from the PRG RAM.
    ///
    /// @param addr the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    virtual uint8_t readPRG (uint16_t addr) = 0;

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param addr the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    virtual void writePRG (uint16_t addr, uint8_t value) = 0;

    /// Read a byte from the CHR RAM.
    ///
    /// @param addr the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    virtual uint8_t readCHR (uint16_t addr) = 0;

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param addr the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    virtual void writeCHR (uint16_t addr, uint8_t value) = 0;

    /// Return the page pointer for the given address.
    ///
    /// @param addr the address of the page pointer to get
    /// @return the page pointer at the given address
    ///
    virtual const uint8_t* getPagePtr (uint16_t addr) = 0; //for DMAs

    /// Return the name table mirroring mode of this mapper.
    virtual NameTableMirroring getNameTableMirroring() {
        return static_cast<NameTableMirroring>(m_cartridge.getNameTableMirroring());
    };

    /// Return true if this mapper has extended RAM, false otherwise.
    bool hasExtendedRAM() { return m_cartridge.hasExtendedRAM(); };

protected:
    /// The cartridge this mapper associates with
    Cartridge& m_cartridge;

};

#endif //MAPPER_H
