#pragma once
#include <iostream>
#include <cstring>
#include "common.hpp"

/// An abstract base class for a Mapper module on a Cartridge
class Mapper {
    /// the ROM this mapper is loading from
    u8* rom;
    /// the size of the ROM in bytes
    int romSize;
    /// whether this mapper has CHR RAM
    bool chrRam = false;

protected:
    u32 prgMap[4];
    u32 chrMap[8];

    u8 *prg, *chr, *prgRam;
    u32 prgSize, chrSize, prgRamSize;

    template <int pageKBs> void map_prg(int slot, int bank);
    template <int pageKBs> void map_chr(int slot, int bank);

public:
    Mapper() { };
    Mapper(u8* rom);
    Mapper(Mapper* mapper);
    virtual Mapper* copy();
    virtual ~Mapper();

    u8 read(u16 addr);
    virtual u8 write(u16 addr, u8 v) { return v; }

    u8 chr_read(u16 addr);
    virtual u8 chr_write(u16 addr, u8 v) { return v; }

    virtual void signal_scanline() {}
};
