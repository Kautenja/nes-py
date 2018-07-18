#include <csignal>
#include <iostream>
#include "cartridge.hpp"
#include "cpu.hpp"
#include "gui.hpp"

/// The graphics interface to SDL
namespace GUI {

    /// Return the width of the screen.
    unsigned get_width() { return WIDTH; }

    /// Return the height of the screen.
    unsigned get_height() { return HEIGHT; }

    /// the pixels representing a frame (the screen)
    u32 screen[HEIGHT][WIDTH];

    /// Send the PPU rendered frame to the SDL GUI.
    void new_frame(u32* pixels) {
        // TODO: vectorize this operation / determine fastest caches access
        for (int h = 0; h < HEIGHT; h++)
            for (int w = 0; w < WIDTH; w++)
                screen[h][w] = *(pixels + h + (w * WIDTH));
    }

    /// Return the screen.
    u32* get_screen() {
        return *screen;
    }



    // /// the pixels representing a frame (the screen)
    // u32* screen;

    // /// Send the PPU rendered frame to the SDL GUI.
    // void new_frame(u32* pixels) {
    //     screen = pixels;
    // }

    // /// Return the screen.
    // u32* get_screen() {
    //     return screen;
    // }




    // /// the pixels representing a frame (the screen)
    // u32 screen;

    // /// Send the PPU rendered frame to the screen.
    // void new_frame(u32* pixels) {
    //     screen = *pixels;
    // }

    // /// Return the screen.
    // u32 get_screen() {
    //     return screen;
    // }

}
