#include <cstdio>
#include "mappers/mapper0.hpp"
#include "mappers/mapper1.hpp"
#include "mappers/mapper2.hpp"
#include "mappers/mapper3.hpp"
#include "mappers/mapper4.hpp"
#include "cartridge.hpp"

namespace Cartridge {

    /// the mapper for the ROM associated with this cartridge
    Mapper* mapper = nullptr;

    /// PRG-ROM access
    template <bool wr> u8 access(u16 addr, u8 v) {
        if (!wr) return mapper->read(addr);
        else     return mapper->write(addr, v);
    }
    template u8 access<0>(u16, u8); template u8 access<1>(u16, u8);

    /// CHR-ROM/RAM access
    template <bool wr> u8 chr_access(u16 addr, u8 v) {
        if (!wr) return mapper->chr_read(addr);
        else     return mapper->chr_write(addr, v);
    }
    template u8 chr_access<0>(u16, u8); template u8 chr_access<1>(u16, u8);

    /// Signal a scanline to the mapper for this cartridge
    void signal_scanline() {
        mapper->signal_scanline();
    }

    /**
        Load the ROM from a file.

        @param fileName the name of the file to load the ROM from

    */
    void load(const char* fileName) {
        FILE* f = fopen(fileName, "rb");

        fseek(f, 0, SEEK_END);
        int size = ftell(f);
        fseek(f, 0, SEEK_SET);

        u8* rom = new u8[size];
        fread(rom, size, 1, f);
        fclose(f);

        int mapperNum = (rom[7] & 0xF0) | (rom[6] >> 4);
        if (loaded()) delete mapper;
        switch (mapperNum) {
            case 0:  mapper = new Mapper0(rom); break;
            case 1:  mapper = new Mapper1(rom); break;
            case 2:  mapper = new Mapper2(rom); break;
            case 3:  mapper = new Mapper3(rom); break;
            case 4:  mapper = new Mapper4(rom); break;
        }
    }

    /// Return true if there is a ROM (mapper) loaded, false otherwise.
    bool loaded() {
        return mapper != nullptr;
    }

}
