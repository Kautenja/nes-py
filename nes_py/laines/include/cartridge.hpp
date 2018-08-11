#pragma once
#include "common.hpp"
#include "mapper.hpp"

/// an abstraction of a game cartridge.
class Cartridge {
private:
    /// the mapper for the ROM associated with this cartridge
    Mapper* mapper;

public:
    /**
        Initialize a new cartridge.

        @param file_name the name of the file to load the ROM from
    */
    Cartridge(const char* file_name);

    /// Initialize a cartridge as a copy of another
    Cartridge(Cartridge* cart) {
        mapper = cart->mapper->copy();
    };

    /// Delete an instance of cartridge
    ~Cartridge();

    /// Signal a scanline to the mapper for this cartridge.
    void signal_scanline();

    /// PRG-ROM access
    template <bool wr> u8 access(u16 addr, u8 v = 0);

    /// CHR-ROM/RAM access
    template <bool wr> u8 chr_access(u16 addr, u8 v = 0);
};
