//  Program:      nes-py
//  File:         cpu_opcodes.hpp
//  Description:  This file defines relevant CPU opcodes
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CPU_OPCODES_HPP
#define CPU_OPCODES_HPP

#include "common.hpp"

namespace NES {

const auto INSTRUCTION_MODE_MASK = 0x3;

const auto OPERATION_MASK = 0xe0;
const auto OPERATION_SHIFT = 5;

const auto ADRESS_MODE_MASK = 0x1c;
const auto ADDRESS_MODE_SHIFT = 2;

const auto BRANCH_INSTRUCTION_MASK = 0x1f;
const auto BRANCH_INSTRUCTION_MASK_RESULT = 0x10;
const auto BRANCH_CONDITION_MASK = 0x20;
const auto BRANCH_ON_FLAG_SHIFT = 6;

const auto NMI_VECTOR = 0xfffa;
const auto RESET_VECTOR = 0xfffc;
const auto IRQ_VECTOR = 0xfffe;

enum BranchOnFlag {
    NEGATIVE_,
    OVERFLOW_,
    CARRY_,
    ZERO_,
};

enum Operation1 {
    ORA,
    AND,
    EOR,
    ADC,
    STA,
    LDA,
    CMP,
    SBC,
};

enum AddrMode1 {
    M1_INDEXED_INDIRECT_X,
    M1_ZERO_PAGE,
    M1_IMMEDIATE,
    M1_ABSOLUTE,
    M1_INDIRECT_Y,
    M1_INDEXED_X,
    M1_ABSOLUTE_Y,
    M1_ABSOLUTE_X,
};

enum Operation2 {
    ASL,
    ROL,
    LSR,
    ROR,
    STX,
    LDX,
    DEC,
    INC,
};

enum AddrMode2 {
    M2_IMMEDIATE,
    M2_ZERO_PAGE,
    M2_ACCUMULATOR,
    M2_ABSOLUTE,
    M2_INDEXED          = 5,
    M2_ABSOLUTE_INDEXED = 7,
};

enum Operation0 {
    BIT  = 1,
    STY  = 4,
    LDY,
    CPY,
    CPX,
};

/// Implied mode opcodes
enum OperationImplied {
    BRK = 0x00,
    PHP = 0x08,
    CLC = 0x18,
    JSR = 0x20,
    PLP = 0x28,
    SEC = 0x38,
    RTI = 0x40,
    PHA = 0x48,
    JMP  = 0x4C,
    CLI = 0x58,
    RTS = 0x60,
    PLA = 0x68,
    JMPI = 0x6C,  // JMP indirect
    SEI = 0x78,
    DEY = 0x88,
    TXA = 0x8a,
    TYA = 0x98,
    TXS = 0x9a,
    TAY = 0xa8,
    TAX = 0xaa,
    CLV = 0xb8,
    TSX = 0xba,
    INY = 0xc8,
    DEX = 0xca,
    CLD = 0xd8,
    INX = 0xe8,
    NOP = 0xea,
    SED = 0xf8,
};

/// A portable structure for working with the 6502 status register.
struct CPU_Flags {
    static const NES_Byte FLAG_CARRY = 0x01;
    static const NES_Byte FLAG_ZERO = 0x02;
    static const NES_Byte FLAG_INTERRUPT_DISABLE = 0x04;
    static const NES_Byte FLAG_DECIMAL = 0x08;
    static const NES_Byte FLAG_BREAK = 0x10;
    static const NES_Byte FLAG_UNUSED = 0x20;
    static const NES_Byte FLAG_OVERFLOW = 0x40;
    static const NES_Byte FLAG_NEGATIVE = 0x80;

    NES_Byte byte;

    CPU_Flags() : byte(FLAG_UNUSED) { }

    inline bool get(NES_Byte mask) const { return (byte & mask) != 0; }

    inline void set(NES_Byte mask, bool value) {
        if (value)
            byte |= mask;
        else
            byte &= static_cast<NES_Byte>(~mask);
    }

    inline void set_byte(NES_Byte value) { byte = value | FLAG_UNUSED; }

    inline NES_Byte status(bool break_flag) const {
        NES_Byte value = byte | FLAG_UNUSED;
        if (break_flag)
            value |= FLAG_BREAK;
        else
            value &= static_cast<NES_Byte>(~FLAG_BREAK);
        return value;
    }

    inline bool C() const { return get(FLAG_CARRY); }
    inline bool Z() const { return get(FLAG_ZERO); }
    inline bool I() const { return get(FLAG_INTERRUPT_DISABLE); }
    inline bool D() const { return get(FLAG_DECIMAL); }
    inline bool V() const { return get(FLAG_OVERFLOW); }
    inline bool N() const { return get(FLAG_NEGATIVE); }

    inline void set_ZN(NES_Byte value) {
        byte &= static_cast<NES_Byte>(~(FLAG_ZERO | FLAG_NEGATIVE));
        if (value == 0)
            byte |= FLAG_ZERO;
        if (value & 0x80)
            byte |= FLAG_NEGATIVE;
        byte |= FLAG_UNUSED;
    }

    inline void set_C(bool value) { set(FLAG_CARRY, value); }
    inline void set_Z(bool value) { set(FLAG_ZERO, value); }
    inline void set_I(bool value) { set(FLAG_INTERRUPT_DISABLE, value); }
    inline void set_D(bool value) { set(FLAG_DECIMAL, value); }
    inline void set_V(bool value) { set(FLAG_OVERFLOW, value); }
    inline void set_N(bool value) { set(FLAG_NEGATIVE, value); }
};

/// a mapping of opcodes to the number of cycles used by the opcode. 0 implies
/// an unused opcode.
const NES_Byte OPERATION_CYCLES[0x100] = {
    7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,
    2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
};

}  // namespace NES

#endif  // CPU_OPCODES_HPP
