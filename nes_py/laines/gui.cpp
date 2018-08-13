#include "gui.hpp"

GUI::GUI() {

};

GUI::GUI(GUI* gui) {
    // copy the screen from the other GUI into this GUI
    memcpy(screen, gui->screen, WIDTH * HEIGHT * sizeof(u32));
};

unsigned GUI::get_width() {
    return GUI::WIDTH;
}

unsigned GUI::get_height() {
    return GUI::HEIGHT;
}

void GUI::new_frame(u32* pixels) {
    // copy the pixels into the screen array (pointer)
    memcpy(this->screen, pixels, this->WIDTH * this->HEIGHT * sizeof(u32));
}

void GUI::copy_screen(unsigned char *output_buffer) {
    // copy the screen into the output buffer
    memcpy(output_buffer, this->screen, this->WIDTH * this->HEIGHT * sizeof(u32));
}
