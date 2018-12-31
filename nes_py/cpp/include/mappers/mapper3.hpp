#pragma once
#include "mapper.hpp"

class Mapper3 : public Mapper {
    u8 regs[1];
    bool vertical_mirroring;
    bool PRG_size_16k;
    void apply();

public:
    Mapper3(Mapper3* mapper): Mapper(mapper) {
        std::copy(std::begin(mapper->regs), std::end(mapper->regs), std::begin(regs));
        vertical_mirroring = mapper->vertical_mirroring;
        PRG_size_16k = mapper->PRG_size_16k;
    };
    Mapper3(u8* rom) : Mapper(rom) {
        PRG_size_16k = rom[4] == 1;
        vertical_mirroring = rom[6] & 0x01;
        regs[0] = 0;
        apply();
    }

    Mapper3* copy() { return new Mapper3(this); };

    u8 write(u16 addr, u8 v);
    u8 chr_write(u16 addr, u8 v);
};

