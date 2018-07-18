#pragma once
#include "common.hpp"

namespace GUI {

    unsigned get_width();
    unsigned get_height();
    void new_frame(u32* pixels);
    u32* get_screen();

}
