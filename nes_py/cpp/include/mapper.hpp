//  Program:      nes-py
//  File:         mapper.hpp
//  Description:  This class provides an abstraction of an NES cartridge mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef MAPPER_HPP
#define MAPPER_HPP

#include "common.hpp"
#include "cartridge.hpp"
#include <functional>

/// Mirroring modes supported by the NES
enum NameTableMirroring {
    HORIZONTAL  = 0,
    VERTICAL    = 1,
    FOUR_SCREEN  = 8,
    ONE_SCREEN_LOWER,
    ONE_SCREEN_HIGHER,
};

/// An abstraction of a general hardware mapper for different NES cartridges
class Mapper {

protected:
    /// The cartridge this mapper associates with
    Cartridge& cartridge;

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
    /// @param game a reference to a cartridge for the mapper to access
    ///
    Mapper(Cartridge& game) : cartridge(game) { };

    /// Create a mapper based on given type, a game cartridge.
    ///
    /// @param game a reference to a cartridge for the mapper to access
    /// @param mirroring_cb the callback to signify a change in mirroring mode
    /// @return a pointer to a mapper class based on the given game
    ///
    static Mapper* create (Cartridge& game, std::function<void(void)> mirroring_cb);

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    virtual NES_Byte readPRG (NES_Address address) = 0;

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    virtual void writePRG (NES_Address address, NES_Byte value) = 0;

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    virtual NES_Byte readCHR (NES_Address address) = 0;

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    virtual void writeCHR (NES_Address address, NES_Byte value) = 0;

    /// Return the page pointer for the given address.
    ///
    /// @param address the address of the page pointer to get
    /// @return the page pointer at the given address
    ///
    virtual const NES_Byte* getPagePtr (NES_Address address) = 0;

    /// Return the name table mirroring mode of this mapper.
    inline virtual NameTableMirroring getNameTableMirroring() {
        return static_cast<NameTableMirroring>(cartridge.getNameTableMirroring());
    };

    /// Return true if this mapper has extended RAM, false otherwise.
    inline bool hasExtendedRAM() { return cartridge.hasExtendedRAM(); };

};

#endif //MAPPER_HPP
