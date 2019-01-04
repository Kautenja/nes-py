#ifndef CPU_H
#define CPU_H
#include "main_bus.hpp"

class CPU
{
public:
    enum InterruptType
    {
        IRQ,
        NMI,
        BRK_
    };

    CPU();

    //Assuming sequential execution, for asynchronously calling this with Execute, further work needed
    void interrupt(MainBus &m_bus, InterruptType type);

    void step(MainBus &m_bus);
    void reset(MainBus &m_bus);
    void reset(Address start_addr);
    void log();

    Address getPC() { return r_PC; }
    void skipDMACycles();
private:
    //Instructions are split into five sets to make decoding easier.
    //These functions return true if they succeed
    bool executeImplied(MainBus &m_bus, Byte opcode);
    bool executeBranch(MainBus &m_bus, Byte opcode);
    bool executeType0(MainBus &m_bus, Byte opcode);
    bool executeType1(MainBus &m_bus, Byte opcode);
    bool executeType2(MainBus &m_bus, Byte opcode);

    Address readAddress(MainBus &m_bus, Address addr);

    void pushStack(MainBus &m_bus, Byte value);
    Byte pullStack(MainBus &m_bus);

    //If a and b are in different pages, increases the m_SkipCycles by inc
    void setPageCrossed(Address a, Address b, int inc = 1);
    void setZN(Byte value);

    int m_skipCycles;
    int m_cycles;

    //Registers
    Address r_PC;
    Byte r_SP;
    Byte r_A;
    Byte r_X;
    Byte r_Y;

    //Status flags.
    //Is storing them in one byte better ?
    bool f_C;
    bool f_Z;
    bool f_I;
//            bool f_B;
    bool f_D;
    bool f_V;
    bool f_N;

};

#endif // CPU_H
