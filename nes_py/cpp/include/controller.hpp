#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <cstdint>
#include <vector>

using Byte = std::uint8_t;

/// A standard NES controller
class Controller {

private:
    /// whether strobe is on
    bool is_strobe;
    /// the emulation of the buttons on the controller
    unsigned int joypad_buttons;
    /// the state of the buttons
    unsigned int joypad_bits;

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
    void write_buttons(uint8_t buttons);

    /// Strobe the controller
    void strobe(Byte b);

    /// Read the controller state
    ///
    /// @return a state from the controller
    ///
    Byte read();

};

#endif // CONTROLLER_H
