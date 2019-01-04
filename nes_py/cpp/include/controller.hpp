#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <SFML/Window.hpp>
#include <cstdint>
#include <vector>

using Byte = std::uint8_t;

/// A standard NES controller
class Controller {

private:
    /// whether strobe is on
    bool m_strobe;
    /// the state of the buttons
    unsigned int m_keyStates;
    /// TODO
    std::vector<sf::Keyboard::Key> m_keyBindings;

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

    /// Strobe the controller
    void strobe(Byte b);

    /// Read the controller state
    Byte read();

};

#endif // CONTROLLER_H
