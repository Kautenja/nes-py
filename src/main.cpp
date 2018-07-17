/**
    The main entry point for the LaiNES command line application.

    @author Andrea Orru
    @author Christian Kauten
*/
#include <iostream>
#include <sys/stat.h>
#include <string>
#include "gui.hpp"


/**
    Print a usage string for this application to the command line.
    @return the code for a failed execution of the application (-1)
*/
int print_usage() {
    std::cout <<
        "Usage: \n\n" <<
        "    laines <path_to_nes_rom>\n" <<
        std::endl;
    return -1;
}


/**
    Return true if the given string points to an existing file.

    @param name the string name of a potential file
    @return true if `name` points to an existing file, false otherwise
*/
inline bool file_exists(const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}


/**
    The main entry point for the C++ application.

    @param argc the number of command line arguments passed
    @param argv the array of string command line arguments
    @return 0 if the application runs successfully, -1 otherwise
*/
int main(int argc, char *argv[]) {
    // ensure the correct number of command line arguments were passed
    if (argc < 2) return print_usage();

    // unwrap the path from the command line and ensure it exists
    std::string path(argv[1]);
    if (not file_exists(path)) return print_usage();

    // initialize the GUI with the given ROM path
    GUI::init(path);
    // run the GUI
    GUI::run();

    return 0;
}
