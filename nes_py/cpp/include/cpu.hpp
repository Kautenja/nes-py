//  Program:      nes-py
//  File:         cpu.hpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CPU_H
#define CPU_H

#include "main_bus.hpp"

/// An MOS6502 for an NES emulation
class CPU {

private:
    int m_skipCycles;
    int m_cycles;

    //Registers
    uint16_t r_PC;
    uint8_t r_SP;
    uint8_t r_A;
    uint8_t r_X;
    uint8_t r_Y;

    // CPU Status flags.
    // TODO: Is storing them in one byte better ?
    bool f_C;
    bool f_Z;
    bool f_I;
    // bool f_B;
    bool f_D;
    bool f_V;
    bool f_N;

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

public:
    /// The interrupt types available to this CPU
    enum InterruptType {
        IRQ,
        NMI,
        BRK_
    };

    /// Initialize a new CPU.
    CPU() { };

    /// Interrupt the CPU.
    ///
    /// @param m_bus the main bus of the machine
    /// @param type the type of interrupt to issue
    ///
    /// TODO: Assuming sequential execution, for asynchronously calling this
    ///       with Execute, further work needed
    ///
    void interrupt(MainBus &m_bus, InterruptType type);

    /// Perform a full CPU cycle using and storing data in the given bus.
    ///
    /// @param m_bus the bus to read and write data from / to
    ///
    void step(MainBus &m_bus);

    /// Reset the emulator using the given starting address.
    ///
    /// @param start_addr the starting address for the program counter
    ///
    void reset(uint16_t start_addr);

    /// Reset using the given main bus to lookup a starting address.
    ///
    /// @param m_bus the main bus of the NES emulator
    ///
    void reset(MainBus &m_bus);

    /// Return the program counter of the CPU.
    uint16_t getPC() { return r_PC; }

    /// Skip DMA cycles.
    void skipDMACycles();

};

#endif // CPU_H
