#include "gui.hpp"

/// Control for the emulator using virtual controllers
namespace Joypad {
    /// Virtual joypad with values from Python API
    u8 joypad_buttons[2];
    /// Joypad shift registers.
    u8 joypad_bits[2];
    /// Joypad strobe latch.
    bool strobe;

    /**
        Write a button state to the given joypad.

        @param n the joypad to address. 0 for player 1, 1 for player 2
        @param buttons the bitmap of pressed buttons on the controller
    */
    void write_buttons(int n, u8 buttons) {
        joypad_buttons[n] = buttons;
    }

    /**
        Read joypad state (NES register format).

        @param n the joypad to address. 0 for player 1, 1 for player 2
        @returns the byte representing the controller state
    */
    u8 read_state(int n) {
        // When strobe is high, it keeps reading A:
        if (strobe)
            return 0x40 | (joypad_buttons[n] & 1);

        // Get the status of a button and shift the register:
        u8 j = 0x40 | (joypad_bits[n] & 1);
        joypad_bits[n] = 0x80 | (joypad_bits[n] >> 1);
        return j;
    }

    /**
        Write joypad strobe flag.

        @param v whether strobe is enabled (true) or disabled (false)
    */
    void write_strobe(bool v) {
        // Read the joypad data on strobe's transition 1 -> 0.
        if (strobe and !v)
            for (int i = 0; i < 2; i++)
                joypad_bits[i] = joypad_buttons[i];

        strobe = v;
    }
}
