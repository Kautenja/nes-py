//  Program:      nes-py
//  File:         controller.hpp
//  Description:  This class houses the logic and data for an NES controller
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#include <vector>

/// A standard NES controller
class Controller {

private:
    /// whether strobe is on
    bool is_strobe;
    /// the emulation of the buttons on the controller
    uint8_t joypad_buttons;
    /// the state of the buttons
    uint8_t joypad_bits;

public:
    /// Initialize a new controller
    Controller();

    /// The buttons on the controller
    enum Buttons {
        A,
        B,
        Select,
        Start,
        Up,
        Down,
        Left,
        Right,
        TotalButtons,
    };

    /// Write buttons to the virtual controller
    ///
    /// @param buttons the button bitmap to write to the controller
    ///
    void write_buttons(uint8_t buttons) { joypad_buttons = buttons; };

    /// Strobe the controller
    void strobe(uint8_t b);

    /// Read the controller state
    ///
    /// @return a state from the controller
    ///
    uint8_t read();

};

#endif // CONTROLLER_H
