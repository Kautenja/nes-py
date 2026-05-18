//  Program:      nes-py
//  File:         cpu.cpp
//  Description:  This class houses the logic and data for the NES CPU
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "nes_emu/cpu.hpp"
#include "nes_emu/log.hpp"
#include <array>

namespace NES {

bool CPU::implied(MainBus &bus, NES_Byte opcode) {
    switch (static_cast<OperationImplied>(opcode)) {
        case BRK: {
            interrupt(bus, BRK_INTERRUPT);
            break;
        }
        case PHP: {
            push_stack(bus, flags.status(true));
            break;
        }
        case CLC: {
            flags.set_C(false);
            break;
        }
        case JSR: {
            // Push address of next instruction - 1, thus register_PC + 1
            // instead of register_PC + 2 since register_PC and
            // register_PC + 1 are address of subroutine
            push_stack(bus, static_cast<NES_Byte>((register_PC + 1) >> 8));
            push_stack(bus, static_cast<NES_Byte>(register_PC + 1));
            register_PC = read_address(bus, register_PC);
            break;
        }
        case PLP: {
            flags.set_byte(pop_stack(bus));
            break;
        }
        case SEC: {
            flags.set_C(true);
            break;
        }
        case RTI: {
            flags.set_byte(pop_stack(bus));
            register_PC = pop_stack(bus);
            register_PC |= pop_stack(bus) << 8;
            break;
        }
        case PHA: {
            push_stack(bus, register_A);
            break;
        }
        case JMP: {
            register_PC = read_address(bus, register_PC);
            break;
        }
        case CLI: {
            flags.set_I(false);
            break;
        }
        case RTS: {
            register_PC = pop_stack(bus);
            register_PC |= pop_stack(bus) << 8;
            ++register_PC;
            break;
        }
        case PLA: {
            register_A = pop_stack(bus);
            set_ZN(register_A);
            break;
        }
        case JMPI: {
            NES_Address location = read_address(bus, register_PC);
            // 6502 has a bug such that the when the vector of an indirect
            // address begins at the last byte of a page, the second byte
            // is fetched from the beginning of that page rather than the
            // beginning of the next
            // Recreating here:
            NES_Address Page = location & 0xff00;
            register_PC = bus.read(location) | bus.read(Page | ((location + 1) & 0xff)) << 8;
            break;
        }
        case SEI: {
            flags.set_I(true);
            break;
        }
        case DEY: {
            --register_Y;
            set_ZN(register_Y);
            break;
        }
        case TXA: {
            register_A = register_X;
            set_ZN(register_A);
            break;
        }
        case TYA: {
            register_A = register_Y;
            set_ZN(register_A);
            break;
        }
        case TXS: {
            register_SP = register_X;
            break;
        }
        case TAY: {
            register_Y = register_A;
            set_ZN(register_Y);
            break;
        }
        case TAX: {
            register_X = register_A;
            set_ZN(register_X);
            break;
        }
        case CLV: {
            flags.set_V(false);
            break;
        }
        case TSX: {
            register_X = register_SP;
            set_ZN(register_X);
            break;
        }
        case INY: {
            ++register_Y;
            set_ZN(register_Y);
            break;
        }
        case DEX: {
            --register_X;
            set_ZN(register_X);
            break;
        }
        case CLD: {
            flags.set_D(false);
            break;
        }
        case INX: {
            ++register_X;
            set_ZN(register_X);
            break;
        }
        case NOP: {
            break;
        }
        case SED: {
            flags.set_D(true);
            break;
        }
        default: return false;
    }
    return true;
}

bool CPU::branch(MainBus &bus, NES_Byte opcode) {
    if ((opcode & BRANCH_INSTRUCTION_MASK) != BRANCH_INSTRUCTION_MASK_RESULT)
        return false;

    // branch is initialized to the condition required (for the flag
    // specified later)
    bool branch = opcode & BRANCH_CONDITION_MASK;

    // set branch to true if the given condition is met by the given flag
    // We use xnor here, it is true if either both operands are true or
    // false
    switch (opcode >> BRANCH_ON_FLAG_SHIFT) {
        case NEGATIVE_: {
            branch = !(branch ^ flags.N());
            break;
        }
        case OVERFLOW_: {
            branch = !(branch ^ flags.V());
            break;
        }
        case CARRY_: {
            branch = !(branch ^ flags.C());
            break;
        }
        case ZERO_: {
            branch = !(branch ^ flags.Z());
            break;
        }
        default: return false;
    }

    if (branch) {
        int8_t offset = bus.read(register_PC++);
        ++skip_cycles;
        auto newPC = static_cast<NES_Address>(register_PC + offset);
        set_page_crossed(register_PC, newPC, 2);
        register_PC = newPC;
    } else {
        ++register_PC;
    }
    return true;
}

bool CPU::type0(MainBus &bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) != 0x0)
        return false;

    NES_Address location = 0;
    switch (static_cast<AddrMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case M2_IMMEDIATE: {
            location = register_PC++;
            break;
        }
        case M2_ZERO_PAGE: {
            location = bus.read(register_PC++);
            break;
        }
        case M2_ABSOLUTE: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        }
        case M2_INDEXED: {
            // Address wraps around in the zero page
            location = (bus.read(register_PC++) + register_X) & 0xff;
            break;
        }
        case M2_ABSOLUTE_INDEXED: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            set_page_crossed(location, location + register_X);
            location += register_X;
            break;
        }
        default: return false;
    }
    switch (static_cast<Operation0>((opcode & OPERATION_MASK) >> OPERATION_SHIFT)) {
        case BIT: {
            NES_Address operand = bus.read(location);
            flags.set_Z((register_A & operand) == 0);
            flags.set_V((operand & 0x40) != 0);
            flags.set_N((operand & 0x80) != 0);
            break;
        }
        case STY: {
            bus.write(location, register_Y);
            break;
        }
        case LDY: {
            register_Y = bus.read(location);
            set_ZN(register_Y);
            break;
        }
        case CPY: {
            NES_Address diff = register_Y - bus.read(location);
            flags.set_C((diff & 0x100) == 0);
            set_ZN(diff);
            break;
        }
        case CPX: {
            NES_Address diff = register_X - bus.read(location);
            flags.set_C((diff & 0x100) == 0);
            set_ZN(diff);
            break;
        }
        default: return false;
    }
    return true;
}

bool CPU::type1(MainBus &bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) != 0x1)
        return false;
    // Location of the operand, could be in RAM
    NES_Address location = 0;
    auto op = static_cast<Operation1>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
    switch (static_cast<AddrMode1>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case M1_INDEXED_INDIRECT_X: {
            NES_Byte zero_address = register_X + bus.read(register_PC++);
            // Addresses wrap in zero page mode, thus pass through a mask
            location = bus.read(zero_address & 0xff) | bus.read((zero_address + 1) & 0xff) << 8;
            break;
        }
        case M1_ZERO_PAGE: {
            location = bus.read(register_PC++);
            break;
        }
        case M1_IMMEDIATE: {
            location = register_PC++;
            break;
        }
        case M1_ABSOLUTE: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        }
        case M1_INDIRECT_Y: {
            NES_Byte zero_address = bus.read(register_PC++);
            location = bus.read(zero_address & 0xff) | bus.read((zero_address + 1) & 0xff) << 8;
            if (op != STA)
                set_page_crossed(location, location + register_Y);
            location += register_Y;
            break;
        }
        case M1_INDEXED_X: {
            // Address wraps around in the zero page
            location = (bus.read(register_PC++) + register_X) & 0xff;
            break;
        }
        case M1_ABSOLUTE_Y: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            if (op != STA)
                set_page_crossed(location, location + register_Y);
            location += register_Y;
            break;
        }
        case M1_ABSOLUTE_X: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            if (op != STA)
                set_page_crossed(location, location + register_X);
            location += register_X;
            break;
        }
        default: return false;
    }

    switch (op) {
        case ORA: {
            register_A |= bus.read(location);
            set_ZN(register_A);
            break;
        }
        case AND: {
            register_A &= bus.read(location);
            set_ZN(register_A);
            break;
        }
        case EOR: {
            register_A ^= bus.read(location);
            set_ZN(register_A);
            break;
        }
        case ADC: {
            NES_Byte operand = bus.read(location);
            NES_Address sum = register_A + operand + flags.C();
            //Carry forward or UNSIGNED overflow
            flags.set_C((sum & 0x100) != 0);
            //SIGNED overflow, would only happen if the sign of sum is
            //different from BOTH the operands
            flags.set_V(((register_A ^ sum) & (operand ^ sum) & 0x80) != 0);
            register_A = static_cast<NES_Byte>(sum);
            set_ZN(register_A);
            break;
        }
        case STA: {
            bus.write(location, register_A);
            break;
        }
        case LDA: {
            register_A = bus.read(location);
            set_ZN(register_A);
            break;
        }
        case CMP: {
            NES_Address diff = register_A - bus.read(location);
            flags.set_C((diff & 0x100) == 0);
            set_ZN(diff);
            break;
        }
        case SBC: {
            //High carry means "no borrow", thus negate and subtract
            NES_Address subtrahend = bus.read(location),
                     diff = register_A - subtrahend - !flags.C();
            //if the ninth bit is 1, the resulting number is negative => borrow => low carry
            flags.set_C((diff & 0x100) == 0);
            //Same as ADC, except instead of the subtrahend,
            //substitute with it's one complement
            flags.set_V(((register_A ^ diff) & (~subtrahend ^ diff) & 0x80) != 0);
            register_A = diff;
            set_ZN(diff);
            break;
        }
        default: return false;
    }
    return true;
}

bool CPU::type2(MainBus &bus, NES_Byte opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) != 2)
        return false;

    NES_Address location = 0;
    auto op = static_cast<Operation2>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
    auto address_mode = static_cast<AddrMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
    switch (address_mode) {
        case M2_IMMEDIATE: {
            location = register_PC++;
            break;
        }
        case M2_ZERO_PAGE: {
            location = bus.read(register_PC++);
            break;
        }
        case M2_ACCUMULATOR: {
            break;
        }
        case M2_ABSOLUTE: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        }
        case M2_INDEXED: {
            location = bus.read(register_PC++);
            NES_Byte index;
            if (op == LDX || op == STX)
                index = register_Y;
            else
                index = register_X;
            //The mask wraps address around zero page
            location = (location + index) & 0xff;
            break;
        }
        case M2_ABSOLUTE_INDEXED: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            NES_Byte index;
            if (op == LDX || op == STX)
                index = register_Y;
            else
                index = register_X;
            set_page_crossed(location, location + index);
            location += index;
            break;
        }
        default: return false;
    }

    NES_Address operand = 0;
    switch (op) {
        case ASL:
        case ROL:
            if (address_mode == M2_ACCUMULATOR) {
                auto prev_C = flags.C();
                flags.set_C((register_A & 0x80) != 0);
                register_A <<= 1;
                //If Rotating, set the bit-0 to the the previous carry
                register_A = register_A | (prev_C && (op == ROL));
                set_ZN(register_A);
            } else {
                auto prev_C = flags.C();
                operand = bus.read(location);
                flags.set_C((operand & 0x80) != 0);
                operand = operand << 1 | (prev_C && (op == ROL));
                set_ZN(operand);
                bus.write(location, operand);
            }
            break;
        case LSR:
        case ROR:
            if (address_mode == M2_ACCUMULATOR) {
                auto prev_C = flags.C();
                flags.set_C((register_A & 1) != 0);
                register_A >>= 1;
                //If Rotating, set the bit-7 to the previous carry
                register_A = register_A | (prev_C && (op == ROR)) << 7;
                set_ZN(register_A);
            } else {
                auto prev_C = flags.C();
                operand = bus.read(location);
                flags.set_C((operand & 1) != 0);
                operand = operand >> 1 | (prev_C && (op == ROR)) << 7;
                set_ZN(operand);
                bus.write(location, operand);
            }
            break;
        case STX: {
            bus.write(location, register_X);
            break;
        }
        case LDX: {
            register_X = bus.read(location);
            set_ZN(register_X);
            break;
        }
        case DEC: {
            auto tmp = bus.read(location) - 1;
            set_ZN(tmp);
            bus.write(location, tmp);
            break;
        }
        case INC: {
            auto tmp = bus.read(location) + 1;
            set_ZN(tmp);
            bus.write(location, tmp);
            break;
        }
        default: return false;
    }
    return true;
}

void CPU::reset(NES_Address start_address) {
    skip_cycles = 0;
    cycles = 0;
    register_A = 0;
    register_X = 0;
    register_Y = 0;
    flags.set_byte(0b00110100);
    register_PC = start_address;
    // documented startup state
    register_SP = 0xfd;
}

void CPU::interrupt(MainBus &bus, InterruptType type) {
    if (flags.I() && type != NMI_INTERRUPT && type != BRK_INTERRUPT)
        return;
    // Add one if BRK, a quirk of 6502
    if (type == BRK_INTERRUPT)
        ++register_PC;
    // push values on to the stack
    push_stack(bus, register_PC >> 8);
    push_stack(bus, register_PC);
    push_stack(bus, flags.status(type == BRK_INTERRUPT));
    // set the interrupt flag
    flags.set_I(true);
    // handle the kind of interrupt
    switch (type) {
        case IRQ_INTERRUPT:
        case BRK_INTERRUPT:
            register_PC = read_address(bus, IRQ_VECTOR);
            break;
        case NMI_INTERRUPT:
            register_PC = read_address(bus, NMI_VECTOR);
            break;
    }
    // add the number of cycles to handle the interrupt
    skip_cycles += 7;
}

CPU::InstructionFamily CPU::instruction_family(NES_Byte opcode) {
    static const std::array<InstructionFamily, 0x100> families = []() {
        std::array<InstructionFamily, 0x100> table{};
        for (int index = 0; index < 0x100; ++index) {
            NES_Byte value = static_cast<NES_Byte>(index);
            switch (static_cast<OperationImplied>(value)) {
                case BRK:
                case PHP:
                case CLC:
                case JSR:
                case PLP:
                case SEC:
                case RTI:
                case PHA:
                case JMP:
                case CLI:
                case RTS:
                case PLA:
                case JMPI:
                case SEI:
                case DEY:
                case TXA:
                case TYA:
                case TXS:
                case TAY:
                case TAX:
                case CLV:
                case TSX:
                case INY:
                case DEX:
                case CLD:
                case INX:
                case NOP:
                case SED:
                    table[index] = IMPLIED_INSTRUCTION;
                    continue;
                default:
                    break;
            }

            if (
                (value & BRANCH_INSTRUCTION_MASK) ==
                BRANCH_INSTRUCTION_MASK_RESULT
            ) {
                table[index] = BRANCH_INSTRUCTION;
            } else {
                switch (value & INSTRUCTION_MODE_MASK) {
                    case 0x1:
                        table[index] = TYPE1_INSTRUCTION;
                        break;
                    case 0x2:
                        table[index] = TYPE2_INSTRUCTION;
                        break;
                    case 0x0:
                        table[index] = TYPE0_INSTRUCTION;
                        break;
                    default:
                        table[index] = INVALID_INSTRUCTION;
                        break;
                }
            }
        }
        return table;
    }();

    return families[opcode];
}

bool CPU::execute_opcode(MainBus &bus, NES_Byte opcode) {
    switch (instruction_family(opcode)) {
        case IMPLIED_INSTRUCTION:
            return implied(bus, opcode);
        case BRANCH_INSTRUCTION:
            return branch(bus, opcode);
        case TYPE1_INSTRUCTION:
            return type1(bus, opcode);
        case TYPE2_INSTRUCTION:
            return type2(bus, opcode);
        case TYPE0_INSTRUCTION:
            return type0(bus, opcode);
        case INVALID_INSTRUCTION:
            return false;
    }
    return false;
}

int CPU::execute_instruction(MainBus &bus) {
    // Match CPU::cycle's ordering: the CPU cycle is counted before the opcode
    // performs bus I/O, which keeps OAM DMA odd/even penalties unchanged.
    ++cycles;

    // This API is normally called only at an instruction boundary. If a caller
    // reaches it during DMA or interrupt stall time, consume one CPU cycle using
    // the same skip counter semantics as CPU::cycle.
    if (skip_cycles-- > 1)
        return 1;

    skip_cycles = 0;
    NES_Byte op = bus.read(register_PC++);
    if (execute_opcode(bus, op))
        skip_cycles += OPERATION_CYCLES[op];
    else
        std::cout << "failed to execute opcode: " << std::hex << +op << std::endl;

    return skip_cycles > 0 ? skip_cycles : 1;
}

void CPU::consume_pending_cycles(int count) {
    if (count <= 0)
        return;

    cycles += count;
    skip_cycles -= count;
    if (skip_cycles < 1)
        skip_cycles = 1;
}

void CPU::cycle(MainBus &bus) {
    // increment the number of cycles
    ++cycles;
    // if in a skip cycle, return
    if (skip_cycles-- > 1)
        return;
    // reset the number of skip cycles to 0
    skip_cycles = 0;
    // read the opcode from the bus and lookup the number of cycles
    NES_Byte op = bus.read(register_PC++);
    if (execute_opcode(bus, op))
        skip_cycles += OPERATION_CYCLES[op];
    else
        std::cout << "failed to execute opcode: " << std::hex << +op << std::endl;
}

}  // namespace NES
