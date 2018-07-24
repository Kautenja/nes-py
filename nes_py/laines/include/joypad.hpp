#pragma once
#include "common.hpp"

/// A joy-pad abstraction for handling virtual button presses
class Joypad {

private:
    /// Virtual joy-pad with values from Python API
    u8 joypad_buttons[2];
    /// joy-pad shift registers.
    u8 joypad_bits[2];
    /// joy-pad strobe latch.
    bool strobe;

public:

    /**
        Write a button state to the given joy-pad.

        @param n the joy-pad to address. 0 for player 1, 1 for player 2
        @param buttons the bitmap of pressed buttons on the controller
    */
    void write_buttons(int n, u8 buttons);

    /**
        Read joy-pad state (NES register format).

        @param n the joy-pad to address. 0 for player 1, 1 for player 2
        @returns the byte representing the controller state
    */
    u8 read_state(int n);

    /**
        Write joy-pad strobe flag.

        @param v whether strobe is enabled (true) or disabled (false)
    */
    void write_strobe(bool v);
};
