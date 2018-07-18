#pragma once
#include <SDL2/SDL.h>
#include <string>
#include "common.hpp"

namespace GUI {

    void init();
    void new_frame(u32* pixels);
    void render();

}
