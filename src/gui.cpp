#include <csignal>
#include <cstring>
#include <iostream>
#include "cartridge.hpp"
#include "cpu.hpp"
#include "gui.hpp"

/// The graphics interface to SDL
namespace GUI {

    /// the width of the screen in pixels
    const unsigned WIDTH = 256;
    /// Return the width of the screen.
    unsigned get_width() { return WIDTH; }
    /// the height of the screen in pixels
    const unsigned HEIGHT = 240;
    /// Return the height of the screen.
    unsigned get_height() { return HEIGHT; }

    /// the pixels representing a frame (the screen)
    u32 screen[HEIGHT][WIDTH];

    /// Send the PPU rendered frame to the SDL GUI.
    void new_frame(u32* pixels) {
        // copy the pixels into the screen array (pointer)
        memcpy(screen, pixels, WIDTH * HEIGHT * sizeof(u32));
    }

    /// Return the screen.
    u32* get_screen() {
        return *screen;
    }

}
