#include "gui.hpp"


int main(int argc, char *argv[]) {
    // initialize the GUI with the given ROM path
    GUI::init(argv[1]);
    // run the GUI
    GUI::run();

    return 0;
}
