#include "controller.hpp"

Controller::Controller() {
    is_strobe = true;
    joypad_buttons = 0;
    joypad_bits = 0;
}

void Controller::write_buttons(uint8_t buttons) {
    joypad_buttons = buttons;
}

void Controller::strobe(Byte b) {
    is_strobe = (b & 1);
    if (!is_strobe) {
        joypad_bits = joypad_buttons;
    }
}

Byte Controller::read() {
    Byte ret;
    if (is_strobe)
        ret = (joypad_buttons & 1);
    else {
        ret = (joypad_bits & 1);
        joypad_bits >>= 1;
    }
    return ret | 0x40;
}
