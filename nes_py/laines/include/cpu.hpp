#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "common.hpp"
#include "joypad.hpp"
#include "cartridge.hpp"

/* Processor flags */
enum Flag {C, Z, I, D, V, N};
/// a class to contain flag register data
class Flags {
    // a private data structure to hold the flags
    bool f[6];

public:

    /** Handle accessing this object using brackets */
    bool& operator[] (const int i) { return f[i]; }

    /** Return the flags as a byte */
    u8 get() {
        return f[C] |
            f[Z] << 1 |
            f[I] << 2 |
            f[D] << 3 |
            1 << 5 |
            f[V] << 6 |
            f[N] << 7;
    }

    /** Set the flags from a full byte */
    void set(u8 p) {
        f[C] = NTH_BIT(p, 0);
        f[Z] = NTH_BIT(p, 1);
        f[I] = NTH_BIT(p, 2);
        f[D] = NTH_BIT(p, 3);
        f[V] = NTH_BIT(p, 6);
        f[N] = NTH_BIT(p, 7);
    }

};

/// A structure to contain all local variables of a CPU for state backup
struct CPUState {
    /// the CPU RAM
    u8 ram[0x800];
    /// the CPU registers
    u8 A, X, Y, S;
    /// the program counter
    u16 PC;
    /// the CPU flags register
    Flags P;
    /// non-mask-able interrupt and interrupt request flag
    bool nmi, irq;

    /// Initialize a new CPU State
    CPUState() {
        P.set(0x04);
        A = X = Y = S = 0x00;
        memset(ram, 0xFF, sizeof(ram));
        nmi = irq = false;
    }

    /// Initialize a new CPU State as a copy of another
    CPUState(CPUState* state) {
        // copy the RAM array into this CPU state
        std::copy(std::begin(state->ram), std::end(state->ram), std::begin(ram));
        // copy the registers
        A = state->A;
        X = state->X;
        Y = state->Y;
        S = state->S;
        // copy the program counter
        PC = state->PC;
        // copy the flags
        P = state->P;
        // copy the interrupt flags
        nmi = state->nmi;
        irq = state->irq;
    }
};

/// The CPU (MOS6502) for the NES
namespace CPU {

    // Interrupt type
    enum IntType { NMI, RESET, IRQ, BRK };
    // Addressing mode
    typedef u16 (*Mode)(void);

    /**
        Set the local joy-pad to a new value

        @param new_joypad the joy-pad pointer to replace the existing pointer
    */
    void set_joypad(Joypad* new_joypad);

    /**
        Return a pointer to this CPU's joy-pad object.

        @returns a pointer to the CPU's joy-pad
    */
    Joypad* get_joypad();

    /// Set the Cartridge instance pointer to a new value.
    void set_cartridge(Cartridge* new_cartridge);

    /// Return the pointer to this PPU's Cartridge instance
    Cartridge* get_cartridge();

    /**
        Return the value of the given memory address.
        This is meant as a public getter to the memory of the machine for RAM hacks.

        @param address the 16-bit address to read from memory
        @returns the byte located at the given address

    */
    u8 read_mem(u16 address);

    /**
        Return the value of the given memory address.
        This is meant as a public getter to the memory of the machine for RAM hacks.

        @param address the 16-bit address to read from memory
        @param value the 8-bit value to write to the given memory address

    */
    void write_mem(u16 address, u8 value);

    /**
        Set the non-maskable interrupt flag.

        @param v the value to set the flag to
    */
    void set_nmi(bool v = true);

    /**
        Set the interrupt request flag.

        @param v the value to set the flag to
    */
    void set_irq(bool v = true);

    /// Turn on the CPU
    void power();

    /// Run the CPU for roughly a frame
    void run_frame();

    /// Return a new CPU state of the CPU variables
    CPUState* get_state();

    /// Restore the CPU variables from a saved state
    void set_state(CPUState* state);
}
