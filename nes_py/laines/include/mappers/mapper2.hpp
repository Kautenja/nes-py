#pragma once
#include "mapper.hpp"

class Mapper2 : public Mapper {
    u8 regs[1];
    bool vertical_mirroring;

    void apply();

public:
    Mapper2(Mapper2* mapper): Mapper(mapper) {
        std::copy(std::begin(mapper->regs), std::end(mapper->regs), std::begin(regs));
        vertical_mirroring = mapper->vertical_mirroring;
    };
    Mapper2(u8* rom) : Mapper(rom) {
        regs[0] = 0;
        vertical_mirroring = rom[6] & 0x01;
        apply();
    }

    Mapper2* copy() { return new Mapper2(this); };

    u8 write(u16 addr, u8 v);
    u8 chr_write(u16 addr, u8 v);
};
