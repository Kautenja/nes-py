//  Program:      nes-py
//  File:         cpu.hpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CPU_HPP
#define CPU_HPP

#include "nes_emu/common.hpp"
#include "nes_emu/cpu_opcodes.hpp"
#include "nes_emu/main_bus.hpp"

namespace NES {

/// The MOS6502 CPU for the Nintendo Entertainment System (NES)
class CPU {
 private:
    /// Instruction families used to avoid repeated failed decoder probes.
    enum InstructionFamily {
        IMPLIED_INSTRUCTION,
        BRANCH_INSTRUCTION,
        TYPE1_INSTRUCTION,
        TYPE2_INSTRUCTION,
        TYPE0_INSTRUCTION,
        UNOFFICIAL_INSTRUCTION,
        INVALID_INSTRUCTION,
    };

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
    CPU_Flags flags;
    /// The number of cycles to skip
    int skip_cycles;
    /// The number of cycles the CPU has run
    int cycles;

    /// Set the zero and negative flags based on the given value.
    ///
    /// @param value the value to set the zero and negative flags using
    ///
    inline void set_ZN(NES_Byte value) {
        flags.set_ZN(value);
    }

    /// Return the decoder family for an opcode.
    static InstructionFamily instruction_family(NES_Byte opcode);

    /// Execute one opcode using the pre-classified instruction family.
    bool execute_opcode(MainBus &bus, NES_Byte opcode);

    /// Read a 16-bit address from the bus given an address.
    ///
    /// @param bus the bus to read data from
    /// @param address the address in memory to read an address from
    /// @return the 16-bit address located at the given memory address
    ///
    inline NES_Address read_address(MainBus &bus, NES_Address address) {
        return bus.read(address) | bus.read(address + 1) << 8;
    }

    /// Push a value onto the stack.
    ///
    /// @param bus the bus to read data from
    /// @param value the value to push onto the stack
    ///
    inline void push_stack(MainBus &bus, NES_Byte value) {
        bus.write(0x100 | register_SP--, value);
    }

    /// Pop a value off the stack.
    ///
    /// @param bus the bus to read data from
    /// @return the value on the top of the stack
    ///
    inline NES_Byte pop_stack(MainBus &bus) {
        return bus.read(0x100 | ++register_SP);
    }

    /// Increment the skip cycles if two addresses refer to different pages.
    ///
    /// @param a an address
    /// @param b another address
    /// @param inc the number of skip cycles to add
    ///
    inline void set_page_crossed(NES_Address a, NES_Address b, int inc = 1) {
        if ((a & 0xff00) != (b & 0xff00)) skip_cycles += inc;
    }

    /// Execute an implied mode instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool implied(MainBus &bus, NES_Byte opcode);

    /// Execute a branch instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool branch(MainBus &bus, NES_Byte opcode);

    /// Execute a type 0 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type0(MainBus &bus, NES_Byte opcode);

    /// Execute a type 1 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type1(MainBus &bus, NES_Byte opcode);

    /// Execute a type 2 instruction.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool type2(MainBus &bus, NES_Byte opcode);

    /// Execute an unofficial 6502 instruction used by some commercial ROMs.
    ///
    /// @param bus the bus to read and write data from and to
    /// @param opcode the opcode of the operation to perform
    /// @return true if the instruction succeeds
    ///
    bool unofficial(MainBus &bus, NES_Byte opcode);

    /// Reset the emulator using the given starting address.
    ///
    /// @param start_address the starting address for the program counter
    ///
    void reset(NES_Address start_address);

 public:
    /// Snapshot of CPU state for native determinism tests.
    struct Snapshot {
        NES_Address register_PC;
        NES_Byte register_SP;
        NES_Byte register_A;
        NES_Byte register_X;
        NES_Byte register_Y;
        NES_Byte flags;
        int skip_cycles;
        int cycles;
    };

    /// The interrupt types available to this CPU
    enum InterruptType {
        IRQ_INTERRUPT,
        NMI_INTERRUPT,
        BRK_INTERRUPT,
    };

    /// Initialize a new CPU.
    CPU() { };

    /// Reset using the given main bus to lookup a starting address.
    ///
    /// @param bus the main bus of the NES emulator
    ///
    inline void reset(MainBus &bus) { reset(read_address(bus, RESET_VECTOR)); }

    /// Interrupt the CPU.
    ///
    /// @param bus the main bus of the machine
    /// @param type the type of interrupt to issue
    ///
    /// TODO: Assuming sequential execution, for asynchronously calling this
    ///       with Execute, further work needed
    ///
    void interrupt(MainBus &bus, InterruptType type);

    /// Perform a full CPU cycle using and storing data in the given bus.
    ///
    /// @param bus the bus to read and write data from / to
    ///
    void cycle(MainBus &bus);

    /// Return true when the next CPU cycle can execute an opcode.
    inline bool can_execute_instruction() const { return skip_cycles <= 1; }

    /// Execute one instruction and return the CPU cycles it scheduled.
    ///
    /// The caller is responsible for advancing PPU and mapper timing for the
    /// returned cycles. This method runs the opcode on the first cycle and
    /// leaves skip_cycles ready for the caller to consume the remaining cycles.
    ///
    /// @param bus the bus to read and write data from / to
    /// @return the number of CPU cycles consumed by the instruction
    ///
    int execute_instruction(MainBus &bus);

    /// Consume pending instruction/stall cycles without executing an opcode.
    ///
    /// @param count the number of CPU cycles already timed externally
    ///
    void consume_pending_cycles(int count);

    /// Return a copy of CPU state for native tests.
    inline Snapshot save_state() const {
        return {
            register_PC,
            register_SP,
            register_A,
            register_X,
            register_Y,
            flags.byte,
            skip_cycles,
            cycles,
        };
    }

    /// Skip DMA cycles.
    ///
    /// 513 = 256 read + 256 write + 1 dummy read
    /// &1 -> +1 if on odd cycle
    ///
    inline void skip_DMA_cycles() { skip_cycles += 513 + (cycles & 1); }
};

}  // namespace NES

#endif  // CPU_HPP
