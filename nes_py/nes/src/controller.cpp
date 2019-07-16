//  Program:      nes-py
//  File:         controller.cpp
//  Description:  This class houses the logic and data for an NES controller
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "controller.hpp"

namespace NES {

NES_Byte Controller::read() {
    NES_Byte ret;
    if (is_strobe) {
        ret = (joypad_buttons & 1);
    } else {
        ret = (joypad_bits & 1);
        joypad_bits >>= 1;
    }
    return ret | 0x40;
}

}  // namespace NES
