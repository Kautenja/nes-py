#include "gui.hpp"

namespace Joypad {

    /// Joypad shift registers.
    u8 joypad_bits[2];
    /// Joypad strobe latch.
    bool strobe;

    /// Read joypad state (NES register format)
    u8 read_state(int n)
    {
        // When strobe is high, it keeps reading A:
        if (strobe)
            return 0x40 | (GUI::get_joypad_state(n) & 1);

        // Get the status of a button and shift the register:
        u8 j = 0x40 | (joypad_bits[n] & 1);
        joypad_bits[n] = 0x80 | (joypad_bits[n] >> 1);
        return j;
    }

    /// Write joypad strobe
    void write_strobe(bool v)
    {
        // Read the joypad data on strobe's transition 1 -> 0.
        if (strobe and !v)
            for (int i = 0; i < 2; i++)
                joypad_bits[i] = GUI::get_joypad_state(i);

        strobe = v;
    }

}
