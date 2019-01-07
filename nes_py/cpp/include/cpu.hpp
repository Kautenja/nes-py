//  Program:      nes-py
//  File:         cpu.hpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CPU_H
#define CPU_H

#include "common.hpp"
#include "main_bus.hpp"

/// An MOS6502 for an NES emulation
class CPU {

private:
    int m_skipCycles;
    int m_cycles;

    // Registers
    NES_Address register_PC;
    NES_Byte register_SP;
    NES_Byte register_A;
    NES_Byte register_X;
    NES_Byte register_Y;

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
    bool executeImplied(MainBus &m_bus, NES_Byte opcode);
    bool executeBranch(MainBus &m_bus, NES_Byte opcode);
    bool executeType0(MainBus &m_bus, NES_Byte opcode);
    bool executeType1(MainBus &m_bus, NES_Byte opcode);
    bool executeType2(MainBus &m_bus, NES_Byte opcode);

    NES_Address readAddress(MainBus &m_bus, NES_Address addr) { return m_bus.read(addr) | m_bus.read(addr + 1) << 8; };
    void pushStack(MainBus &m_bus, NES_Byte value) { m_bus.write(0x100 | register_SP--, value); };
    NES_Byte pullStack(MainBus &m_bus) { return m_bus.read(0x100 | ++register_SP); };

    //If a and b are in different pages, increases the m_SkipCycles by inc
    void setPageCrossed(NES_Address a, NES_Address b, int inc = 1);
    void setZN(NES_Byte value) { f_Z = !value; f_N = value & 0x80; };

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
    void reset(NES_Address start_addr);

    /// Reset using the given main bus to lookup a starting address.
    ///
    /// @param m_bus the main bus of the NES emulator
    ///
    void reset(MainBus &m_bus);

    /// Return the program counter of the CPU.
    NES_Address getPC() { return register_PC; }

    /// Skip DMA cycles.
    ///
    /// 513 = 256 read + 256 write + 1 dummy read
    /// &1 -> +1 if on odd cycle
    ///
    void skipDMACycles() { m_skipCycles += 513 + (m_cycles & 1); };

};

#endif // CPU_H
