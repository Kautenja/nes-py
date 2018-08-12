#pragma once
#include <iostream>
#include <cstring>
#include "common.hpp"

/// an abstraction of a GUI for copying screens to a high-level client.
class GUI {
private:
    /// the width of the screen in pixels
    static const unsigned WIDTH = 256;

    /// the height of the screen in pixels
    static const unsigned HEIGHT = 240;

    /// the pixels representing a frame (the screen)
    u32 screen[HEIGHT][WIDTH];

public:
    /// Initialize a new GUI.
    GUI();

    /// Initialize a new GUI as a copy of another GUI.
    GUI(GUI* gui);

    /// Return the width of the screen.
    static unsigned get_width();

    /// Return the height of the screen.
    static unsigned get_height();

    /**
        Copy the pixels to the local screen memory.

        @param pixels the pixels representing the screen to copy
    */
    void new_frame(u32* pixels);

    /**
        Copy the screen into an output buffer of unsigned bytes.

        @param output_buffer the pointer to the output buffer
    */
    void copy_screen(unsigned char *output_buffer);
};
