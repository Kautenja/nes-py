#include "gui.hpp"

unsigned GUI::get_width() { return GUI::WIDTH; }

unsigned GUI::get_height() { return GUI::HEIGHT; }

void GUI::new_frame(u32* pixels) {
    // copy the pixels into the screen array (pointer)
    memcpy(this->screen, pixels, this->WIDTH * this->HEIGHT * sizeof(u32));
}

void GUI::copy_screen(unsigned char *output_buffer) {
    memcpy(output_buffer, this->screen, this->WIDTH * this->HEIGHT * sizeof(u32));
}
