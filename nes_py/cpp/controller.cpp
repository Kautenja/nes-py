//  Program:      nes-py
//  File:         controller.cpp
//  Description:  This class houses the logic and data for an NES controller
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "controller.hpp"

Controller::Controller() {
    is_strobe = true;
    joypad_buttons = 0;
    joypad_bits = 0;
}

void Controller::strobe(uint8_t b) {
    is_strobe = (b & 1);
    if (!is_strobe) {
        joypad_bits = joypad_buttons;
    }
}

uint8_t Controller::read() {
    uint8_t ret;
    if (is_strobe)
        ret = (joypad_buttons & 1);
    else {
        ret = (joypad_bits & 1);
        joypad_bits >>= 1;
    }
    return ret | 0x40;
}
