#pragma once
#include "common.hpp"

namespace GUI {

    /// the width of the screen in pixels
    const unsigned WIDTH = 256;

    /// the height of the screen in pixels
    const unsigned HEIGHT = 240;

    unsigned get_width();
    unsigned get_height();
    void new_frame(u32* pixels);
    u32* get_screen();

}
