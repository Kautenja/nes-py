#ifndef CPU_H
#define CPU_H
#include "main_bus.hpp"

/// An MOS6502 for an NES emulation
class CPU {

private:
    //Instructions are split into five sets to make decoding easier.
    //These functions return true if they succeed
    bool executeImplied(MainBus &m_bus, uint8_t opcode);
    bool executeBranch(MainBus &m_bus, uint8_t opcode);
    bool executeType0(MainBus &m_bus, uint8_t opcode);
    bool executeType1(MainBus &m_bus, uint8_t opcode);
    bool executeType2(MainBus &m_bus, uint8_t opcode);

    uint16_t readAddress(MainBus &m_bus, uint16_t addr) { return m_bus.read(addr) | m_bus.read(addr + 1) << 8; };
    void pushStack(MainBus &m_bus, uint8_t value) {  m_bus.write(0x100 | r_SP--, value); };
    uint8_t pullStack(MainBus &m_bus) { return m_bus.read(0x100 | ++r_SP); };

    //If a and b are in different pages, increases the m_SkipCycles by inc
    void setPageCrossed(uint16_t a, uint16_t b, int inc = 1);
    void setZN(uint8_t value) { f_Z = !value; f_N = value & 0x80; };

    int m_skipCycles;
    int m_cycles;

    //Registers
    uint16_t r_PC;
    uint8_t r_SP;
    uint8_t r_A;
    uint8_t r_X;
    uint8_t r_Y;

    //Status flags.
    //Is storing them in one byte better ?
    bool f_C;
    bool f_Z;
    bool f_I;
//            bool f_B;
    bool f_D;
    bool f_V;
    bool f_N;

public:
    enum InterruptType {
        IRQ,
        NMI,
        BRK_
    };

    /// Initialize a new CPU.
    CPU() { };

    //Assuming sequential execution, for asynchronously calling this with Execute, further work needed
    void interrupt(MainBus &m_bus, InterruptType type);

    void step(MainBus &m_bus);
    void reset(MainBus &m_bus);
    void reset(uint16_t start_addr);
    void log();

    uint16_t getPC() { return r_PC; }
    void skipDMACycles();

};

#endif // CPU_H
