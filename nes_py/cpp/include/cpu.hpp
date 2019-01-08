//  Program:      nes-py
//  File:         cpu.hpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CPU_HPP
#define CPU_HPP

#include "common.hpp"
#include "main_bus.hpp"

/// A structure for working with the flags register
typedef union {
    struct {
        bool N : 1,
             V : 1,
             ONE : 1,
             B : 1,
             D : 1,
             I : 1,
             Z : 1,
             C : 1;
    } bits;
    NES_Byte byte;
} Flags;

/// The MOS6502 CPU for the Nintendo Entertainment System (NES)
class CPU {

private:
    /// The program counter register
    NES_Address register_PC;

    /// The stack pointer register
    NES_Byte register_SP;

    /// The A register
    NES_Byte register_A;

    /// The X register
    NES_Byte register_X;

    /// The Y register
    NES_Byte register_Y;

    /// The flags register
    Flags flags;

    /// The number of cycles to skip
    int skip_cycles;

    /// The number of cycles the CPU has run
    int cycles;

    /// Read a 16-bit address from the bus given an address.
    ///
    /// @param bus the bus to read data from
    /// @param address the address in memory to read an address from
    /// @return the 16-bit address located at the given memory address
    ///
    inline NES_Address readAddress(MainBus &bus, NES_Address address) { return bus.read(address) | bus.read(address + 1) << 8; };

    /// Push a value onto the stack.
    ///
    /// @param bus the bus to read data from
    /// @param value the value to push onto the stack
    ///
    inline void pushStack(MainBus &bus, NES_Byte value) { bus.write(0x100 | register_SP--, value); };

    /// Pop a value off the stack.
    ///
    /// @param bus the bus to read data from
    /// @return the value on the top of the stack
    ///
    inline NES_Byte pullStack(MainBus &bus) { return bus.read(0x100 | ++register_SP); };

    //Instructions are split into five sets to make decoding easier.
    //These functions return true if they succeed

    bool executeImplied(MainBus &bus, NES_Byte opcode);
    bool executeBranch(MainBus &bus, NES_Byte opcode);
    bool executeType0(MainBus &bus, NES_Byte opcode);
    bool executeType1(MainBus &bus, NES_Byte opcode);
    bool executeType2(MainBus &bus, NES_Byte opcode);

    //If a and b are in different pages, increases the skip_cycles by inc
    void setPageCrossed(NES_Address a, NES_Address b, int inc = 1);

    /// Set the zero and negative flags based on the given value.
    ///
    /// @param value the value to set the zero and negative flags using
    ///
    inline void setZN(NES_Byte value) { flags.bits.Z = !value; flags.bits.N = value & 0x80; };

    /// Reset the emulator using the given starting address.
    ///
    /// @param start_address the starting address for the program counter
    ///
    void reset(NES_Address start_address);

public:
    /// The interrupt types available to this CPU
    enum InterruptType {
        IRQ_INTERRUPT,
        NMI_INTERRUPT,
        BRK_INTERRUPT,
    };

    /// Initialize a new CPU.
    CPU() { };

    /// Interrupt the CPU.
    ///
    /// @param bus the main bus of the machine
    /// @param type the type of interrupt to issue
    ///
    /// TODO: Assuming sequential execution, for asynchronously calling this
    ///       with Execute, further work needed
    ///
    void interrupt(MainBus &bus, InterruptType type);

    /// Reset using the given main bus to lookup a starting address.
    ///
    /// @param bus the main bus of the NES emulator
    ///
    void reset(MainBus &bus);

    /// Perform a full CPU cycle using and storing data in the given bus.
    ///
    /// @param bus the bus to read and write data from / to
    ///
    void step(MainBus &bus);

    /// Skip DMA cycles.
    ///
    /// 513 = 256 read + 256 write + 1 dummy read
    /// &1 -> +1 if on odd cycle
    ///
    void skipDMACycles() { skip_cycles += 513 + (cycles & 1); };

};

#endif // CPU_HPP
