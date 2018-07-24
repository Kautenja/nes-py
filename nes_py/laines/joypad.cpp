#include "joypad.hpp"

void Joypad::write_buttons(int n, u8 buttons) {
    this->joypad_buttons[n] = buttons;
}

u8 Joypad::read_state(int n) {
    // When strobe is high, it keeps reading A:
    if (this->strobe)
        return 0x40 | (this->joypad_buttons[n] & 1);

    // Get the status of a button and shift the register:
    u8 j = 0x40 | (this->joypad_bits[n] & 1);
    this->joypad_bits[n] = 0x80 | (this->joypad_bits[n] >> 1);
    return j;
}

void Joypad::write_strobe(bool v) {
    // Read the joy-pad data on strobe's transition 1 -> 0.
    if (this->strobe and !v)
        for (int i = 0; i < 2; i++)
            this->joypad_bits[i] = this->joypad_buttons[i];

    this->strobe = v;
}
