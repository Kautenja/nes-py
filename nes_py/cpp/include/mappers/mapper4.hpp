#pragma once
#include "mapper.hpp"

class Mapper4 : public Mapper {
    u8 reg8000;
    u8 regs[8];
    bool horizMirroring;

    u8 irqPeriod;
    u8 irqCounter;
    bool irqEnabled;

    void apply();

public:
    Mapper4(Mapper4* mapper): Mapper(mapper) {
        reg8000 = mapper->reg8000;
        std::copy(std::begin(mapper->regs), std::end(mapper->regs), std::begin(regs));
        horizMirroring = mapper->horizMirroring;
        irqPeriod = mapper->irqPeriod;
        irqCounter = mapper->irqCounter;
        irqEnabled = mapper->irqEnabled;
    };
    Mapper4(u8* rom) : Mapper(rom) {
        for (int i = 0; i < 8; i++)
            regs[i] = 0;

        horizMirroring = true;
        irqEnabled = false;
        irqPeriod = irqCounter = 0;

        map_prg<8>(3, -1);
        apply();
    }

    Mapper4* copy() { return new Mapper4(this); };

    u8 write(u16 addr, u8 v);
    u8 chr_write(u16 addr, u8 v);

    void signal_scanline();
};
