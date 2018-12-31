#pragma once
#include "mapper.hpp"

class Mapper1 : public Mapper {
    int writeN;
    u8 tmpReg;
    u8 regs[4];

    void apply();

public:
    Mapper1(Mapper1* mapper): Mapper(mapper) {
        writeN = mapper->writeN;
        tmpReg = mapper->tmpReg;
        std::copy(std::begin(mapper->regs), std::end(mapper->regs), std::begin(regs));
    };
    Mapper1(u8* rom) : Mapper(rom) {
        regs[0] = 0x0C;
        writeN = tmpReg = regs[1] = regs[2] = regs[3] = 0;
        apply();
    }

    Mapper1* copy() { return new Mapper1(this); };

    u8 write(u16 addr, u8 v);
    u8 chr_write(u16 addr, u8 v);
};
