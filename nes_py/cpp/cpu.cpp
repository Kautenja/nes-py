//  Program:      nes-py
//  File:         cpu.cpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "cpu.hpp"
#include "cpu_opcodes.hpp"
#include "log.hpp"

void CPU::reset(NES_Address start_addr) {
    m_skipCycles = m_cycles = 0;
    register_A = register_X = register_Y = 0;
    f_I = true;
    f_C = f_D = f_N = f_V = f_Z = false;
    register_PC = start_addr;
    register_SP = 0xfd; //documented startup state
}

void CPU::reset(MainBus &m_bus) { reset(readAddress(m_bus, RESET_VECTOR)); }

void CPU::interrupt(MainBus &m_bus, InterruptType type) {
    if (f_I && type != NMI && type != BRK_)
        return;

    if (type == BRK_) //Add one if BRK, a quirk of 6502
        ++register_PC;

    pushStack(m_bus, register_PC >> 8);
    pushStack(m_bus, register_PC);

    NES_Byte flags = f_N << 7 |
                 f_V << 6 |
                   1 << 5 | //unused bit, supposed to be always 1
      (type == BRK_) << 4 | //B flag set if BRK
                 f_D << 3 |
                 f_I << 2 |
                 f_Z << 1 |
                 f_C;
    pushStack(m_bus, flags);

    f_I = true;

    switch (type) {
        case IRQ:
        case BRK_:
            register_PC = readAddress(m_bus, IRQ_VECTOR);
            break;
        case NMI:
            register_PC = readAddress(m_bus, NMI_VECTOR);
            break;
    }

    m_skipCycles += 7;
}

void CPU::setPageCrossed(NES_Address a, NES_Address b, int inc) {
    //Page is determined by the high byte
    if ((a & 0xff00) != (b & 0xff00))
        m_skipCycles += inc;
}

void CPU::step(MainBus &m_bus) {
    ++m_cycles;

    if (m_skipCycles-- > 1)
        return;

    m_skipCycles = 0;

    // int psw =    f_N << 7 |
    //              f_V << 6 |
    //                1 << 5 |
    //              f_D << 3 |
    //              f_I << 2 |
    //              f_Z << 1 |
    //              f_C;
    // std::cout << std::hex << std::setfill('0') << std::uppercase
    //           << std::setw(4) << +register_PC
    //           << "  "
    //           << std::setw(2) << +m_bus.read(register_PC)
    //           << "  "
    //           << "A:"   << std::setw(2) << +register_A << " "
    //           << "X:"   << std::setw(2) << +register_X << " "
    //           << "Y:"   << std::setw(2) << +register_Y << " "
    //           << "P:"   << std::setw(2) << psw << " "
    //           << "SP:"  << std::setw(2) << +register_SP  << /*std::endl;*/" "
    //           << "CYC:" << std::setw(3) << std::setfill(' ') << std::dec << ((m_cycles - 1) * 3) % 341
    //           << std::endl;

    NES_Byte opcode = m_bus.read(register_PC++);
    auto CycleLength = OperationCycles[opcode];

    // Using short-circuit evaluation, call the other function only if the
    // first failed. ExecuteImplied must be called first and ExecuteBranch
    // must be before ExecuteType0
    if (CycleLength && (executeImplied(m_bus, opcode) || executeBranch(m_bus, opcode) ||
                    executeType1(m_bus, opcode) || executeType2(m_bus, opcode) || executeType0(m_bus, opcode)))
        m_skipCycles += CycleLength;
    else
        std::cout << "Unrecognized opcode: " << std::hex << +opcode << std::endl;
}

bool CPU::executeImplied(MainBus &m_bus, NES_Byte opcode) {
    switch (static_cast<OperationImplied>(opcode)) {
        case NOP:
            break;
        case BRK:
            interrupt(m_bus, BRK_);
            break;
        case JSR:
            //Push address of next instruction - 1, thus register_PC + 1 instead of register_PC + 2
            //since register_PC and register_PC + 1 are address of subroutine
            pushStack(m_bus, static_cast<NES_Byte>((register_PC + 1) >> 8));
            pushStack(m_bus, static_cast<NES_Byte>(register_PC + 1));
            register_PC = readAddress(m_bus, register_PC);
            break;
        case RTS:
            register_PC = pullStack(m_bus);
            register_PC |= pullStack(m_bus) << 8;
            ++register_PC;
            break;
        case RTI: {
                NES_Byte flags = pullStack(m_bus);
                f_N = flags & 0x80;
                f_V = flags & 0x40;
                f_D = flags & 0x8;
                f_I = flags & 0x4;
                f_Z = flags & 0x2;
                f_C = flags & 0x1;
            }
            register_PC = pullStack(m_bus);
            register_PC |= pullStack(m_bus) << 8;
            break;
        case JMP:
            register_PC = readAddress(m_bus, register_PC);
            break;
        case JMPI: {
                NES_Address location = readAddress(m_bus, register_PC);
                //6502 has a bug such that the when the vector of anindirect address begins at the last byte of a page,
                //the second byte is fetched from the beginning of that page rather than the beginning of the next
                //Recreating here:
                NES_Address Page = location & 0xff00;
                register_PC = m_bus.read(location) |
                       m_bus.read(Page | ((location + 1) & 0xff)) << 8;
            }
            break;
        case PHP: {
                NES_Byte flags = f_N << 7 |
                             f_V << 6 |
                               1 << 5 | //supposed to always be 1
                               1 << 4 | //PHP pushes with the B flag as 1, no matter what
                             f_D << 3 |
                             f_I << 2 |
                             f_Z << 1 |
                             f_C;
                pushStack(m_bus, flags);
            }
            break;
        case PLP: {
                NES_Byte flags = pullStack(m_bus);
                f_N = flags & 0x80;
                f_V = flags & 0x40;
                f_D = flags & 0x8;
                f_I = flags & 0x4;
                f_Z = flags & 0x2;
                f_C = flags & 0x1;
            }
            break;
        case PHA:
            pushStack(m_bus, register_A);
            break;
        case PLA:
            register_A = pullStack(m_bus);
            setZN(register_A);
            break;
        case DEY:
            --register_Y;
            setZN(register_Y);
            break;
        case DEX:
            --register_X;
            setZN(register_X);
            break;
        case TAY:
            register_Y = register_A;
            setZN(register_Y);
            break;
        case INY:
            ++register_Y;
            setZN(register_Y);
            break;
        case INX:
            ++register_X;
            setZN(register_X);
            break;
        case CLC:
            f_C = false;
            break;
        case SEC:
            f_C = true;
            break;
        case CLI:
            f_I = false;
            break;
        case SEI:
            f_I = true;
            break;
        case CLD:
            f_D = false;
            break;
        case SED:
            f_D = true;
            break;
        case TYA:
            register_A = register_Y;
            setZN(register_A);
            break;
        case CLV:
            f_V = false;
            break;
        case TXA:
            register_A = register_X;
            setZN(register_A);
            break;
        case TXS:
            register_SP = register_X;
            break;
        case TAX:
            register_X = register_A;
            setZN(register_X);
            break;
        case TSX:
            register_X = register_SP;
            setZN(register_X);
            break;
        default:
            return false;
    };
    return true;
}

bool CPU::executeBranch(MainBus &m_bus, NES_Byte opcode) {
    if ((opcode & BRANCH_INSTRUCTION_MASK) == BRANCH_INSTRUCTION_MASK_RESULT) {
        //branch is initialized to the condition required (for the flag specified later)
        bool branch = opcode & BRANCH_CONDITION_MASK;

        //set branch to true if the given condition is met by the given flag
        //We use xnor here, it is true if either both operands are true or false
        switch (opcode >> BRANCH_ON_FLAG_SHIFT) {
            case Negative:
                branch = !(branch ^ f_N);
                break;
            case Overflow:
                branch = !(branch ^ f_V);
                break;
            case Carry:
                branch = !(branch ^ f_C);
                break;
            case Zero:
                branch = !(branch ^ f_Z);
                break;
            default:
                return false;
        }

        if (branch) {
            int8_t offset = m_bus.read(register_PC++);
            ++m_skipCycles;
            auto newPC = static_cast<NES_Address>(register_PC + offset);
            setPageCrossed(register_PC, newPC, 2);
            register_PC = newPC;
        }
        else
            ++register_PC;
        return true;
    }
    return false;
}

bool CPU::executeType1(MainBus &m_bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) == 0x1) {
        NES_Address location = 0; //Location of the operand, could be in RAM
        auto op = static_cast<Operation1>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
        switch (static_cast<AddrMode1>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
            case IndexedIndirectX: {
                    NES_Byte zero_addr = register_X + m_bus.read(register_PC++);
                    //Addresses wrap in zero page mode, thus pass through a mask
                    location = m_bus.read(zero_addr & 0xff) | m_bus.read((zero_addr + 1) & 0xff) << 8;
                }
                break;
            case ZeroPage:
                location = m_bus.read(register_PC++);
                break;
            case Immediate:
                location = register_PC++;
                break;
            case Absolute:
                location = readAddress(m_bus, register_PC);
                register_PC += 2;
                break;
            case IndirectY: {
                    NES_Byte zero_addr = m_bus.read(register_PC++);
                    location = m_bus.read(zero_addr & 0xff) | m_bus.read((zero_addr + 1) & 0xff) << 8;
                    if (op != STA)
                        setPageCrossed(location, location + register_Y);
                    location += register_Y;
                }
                break;
            case IndexedX:
                // Address wraps around in the zero page
                location = (m_bus.read(register_PC++) + register_X) & 0xff;
                break;
            case AbsoluteY:
                location = readAddress(m_bus, register_PC);
                register_PC += 2;
                if (op != STA)
                    setPageCrossed(location, location + register_Y);
                location += register_Y;
                break;
            case AbsoluteX:
                location = readAddress(m_bus, register_PC);
                register_PC += 2;
                if (op != STA)
                    setPageCrossed(location, location + register_X);
                location += register_X;
                break;
            default:
                return false;
        }

        switch (op) {
            case ORA:
                register_A |= m_bus.read(location);
                setZN(register_A);
                break;
            case AND:
                register_A &= m_bus.read(location);
                setZN(register_A);
                break;
            case EOR:
                register_A ^= m_bus.read(location);
                setZN(register_A);
                break;
            case ADC: {
                    NES_Byte operand = m_bus.read(location);
                    NES_Address sum = register_A + operand + f_C;
                    //Carry forward or UNSIGNED overflow
                    f_C = sum & 0x100;
                    //SIGNED overflow, would only happen if the sign of sum is
                    //different from BOTH the operands
                    f_V = (register_A ^ sum) & (operand ^ sum) & 0x80;
                    register_A = static_cast<NES_Byte>(sum);
                    setZN(register_A);
                }
                break;
            case STA:
                m_bus.write(location, register_A);
                break;
            case LDA:
                register_A = m_bus.read(location);
                setZN(register_A);
                break;
            case SBC: {
                    //High carry means "no borrow", thus negate and subtract
                    NES_Address subtrahend = m_bus.read(location),
                             diff = register_A - subtrahend - !f_C;
                    //if the ninth bit is 1, the resulting number is negative => borrow => low carry
                    f_C = !(diff & 0x100);
                    //Same as ADC, except instead of the subtrahend,
                    //substitute with it's one complement
                    f_V = (register_A ^ diff) & (~subtrahend ^ diff) & 0x80;
                    register_A = diff;
                    setZN(diff);
                }
                break;
            case CMP: {
                    NES_Address diff = register_A - m_bus.read(location);
                    f_C = !(diff & 0x100);
                    setZN(diff);
                }
                break;
            default:
                return false;
        }
        return true;
    }
    return false;
}

bool CPU::executeType2(MainBus &m_bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) == 2) {
        NES_Address location = 0;
        auto op = static_cast<Operation2>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
        auto addr_mode =
                static_cast<AddrMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
        switch (addr_mode) {
            case Immediate_:
                location = register_PC++;
                break;
            case ZeroPage_:
                location = m_bus.read(register_PC++);
                break;
            case Accumulator:
                break;
            case Absolute_:
                location = readAddress(m_bus, register_PC);
                register_PC += 2;
                break;
            case Indexed: {
                    location = m_bus.read(register_PC++);
                    NES_Byte index;
                    if (op == LDX || op == STX)
                        index = register_Y;
                    else
                        index = register_X;
                    //The mask wraps address around zero page
                    location = (location + index) & 0xff;
                }
                break;
            case AbsoluteIndexed: {
                    location = readAddress(m_bus, register_PC);
                    register_PC += 2;
                    NES_Byte index;
                    if (op == LDX || op == STX)
                        index = register_Y;
                    else
                        index = register_X;
                    setPageCrossed(location, location + index);
                    location += index;
                }
                break;
            default:
                return false;
        }

        NES_Address operand = 0;
        switch (op) {
            case ASL:
            case ROL:
                if (addr_mode == Accumulator) {
                    auto prev_C = f_C;
                    f_C = register_A & 0x80;
                    register_A <<= 1;
                    //If Rotating, set the bit-0 to the the previous carry
                    register_A = register_A | (prev_C && (op == ROL));
                    setZN(register_A);
                }
                else {
                    auto prev_C = f_C;
                    operand = m_bus.read(location);
                    f_C = operand & 0x80;
                    operand = operand << 1 | (prev_C && (op == ROL));
                    setZN(operand);
                    m_bus.write(location, operand);
                }
                break;
            case LSR:
            case ROR:
                if (addr_mode == Accumulator) {
                    auto prev_C = f_C;
                    f_C = register_A & 1;
                    register_A >>= 1;
                    //If Rotating, set the bit-7 to the previous carry
                    register_A = register_A | (prev_C && (op == ROR)) << 7;
                    setZN(register_A);
                }
                else {
                    auto prev_C = f_C;
                    operand = m_bus.read(location);
                    f_C = operand & 1;
                    operand = operand >> 1 | (prev_C && (op == ROR)) << 7;
                    setZN(operand);
                    m_bus.write(location, operand);
                }
                break;
            case STX:
                m_bus.write(location, register_X);
                break;
            case LDX:
                register_X = m_bus.read(location);
                setZN(register_X);
                break;
            case DEC: {
                    auto tmp = m_bus.read(location) - 1;
                    setZN(tmp);
                    m_bus.write(location, tmp);
                }
                break;
            case INC: {
                    auto tmp = m_bus.read(location) + 1;
                    setZN(tmp);
                    m_bus.write(location, tmp);
                }
                break;
            default:
                return false;
        }
        return true;
    }
    return false;
}

bool CPU::executeType0(MainBus &m_bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) == 0x0) {
        NES_Address location = 0;
        switch (static_cast<AddrMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
            case Immediate_:
                location = register_PC++;
                break;
            case ZeroPage_:
                location = m_bus.read(register_PC++);
                break;
            case Absolute_:
                location = readAddress(m_bus, register_PC);
                register_PC += 2;
                break;
            case Indexed:
                // Address wraps around in the zero page
                location = (m_bus.read(register_PC++) + register_X) & 0xff;
                break;
            case AbsoluteIndexed:
                location = readAddress(m_bus, register_PC);
                register_PC += 2;
                setPageCrossed(location, location + register_X);
                location += register_X;
                break;
            default:
                return false;
        }
        NES_Address operand = 0;
        switch (static_cast<Operation0>((opcode & OPERATION_MASK) >> OPERATION_SHIFT)) {
            case BIT:
                operand = m_bus.read(location);
                f_Z = !(register_A & operand);
                f_V = operand & 0x40;
                f_N = operand & 0x80;
                break;
            case STY:
                m_bus.write(location, register_Y);
                break;
            case LDY:
                register_Y = m_bus.read(location);
                setZN(register_Y);
                break;
            case CPY: {
                    NES_Address diff = register_Y - m_bus.read(location);
                    f_C = !(diff & 0x100);
                    setZN(diff);
                }
                break;
            case CPX: {
                    NES_Address diff = register_X - m_bus.read(location);
                    f_C = !(diff & 0x100);
                    setZN(diff);
                }
                break;
            default:
                return false;
        }

        return true;
    }
    return false;
}
