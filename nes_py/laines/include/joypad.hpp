#pragma once
#include "common.hpp"

namespace Joypad {

    unsigned get_num_buttons();
    void write_buttons(int n, u8 buttons);
    u8 read_state(int n);
    void write_strobe(bool v);

}
