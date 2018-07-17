#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <Nes_Apu.h>
#include "common.hpp"

namespace GUI {

    void init(std::string path);
    void run();

    u8 get_joypad_state(int n);
    void new_frame(u32* pixels);

}
