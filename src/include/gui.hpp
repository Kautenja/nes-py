#pragma once
#include <SDL2/SDL.h>
#include <string>
#include "common.hpp"

namespace GUI {

    unsigned get_width();
    unsigned get_height();

    void init();
    void new_frame(u32* pixels);
    void render();

}
