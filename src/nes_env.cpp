// TODO define header file and cleanup this massive file
// TODO consider defining the extern "C" in a separate file?
// TODO organize imports

// #include <iostream>
#include <string>

#include <SDL2/SDL.h>
// #include <SDL2/SDL_image.h>

#include "cartridge.hpp"
#include "cpu.hpp"
#include "gui.hpp"

/// An OpenAI Gym like interface to the LaiNES emulator.
class NESEnv{

private:
    /// the path to the ROM for the emulator to run
    std::string path;

public:

    /**
        Initialize a new NESEnv.

        @param path the path to the ROM for the emulator to load
        @returns a new instance of NESEnv for a given ROM

    */
    NESEnv(wchar_t* path) {
        // convert the wchar_t type to a string
        std::wstring ws(path);
        std::string str(ws.begin(), ws.end());
        this->path = str;
        GUI::init();
    }

    /// Reset the emulator to its initial state.
    void reset() {
        // load the cartridge based on the environment's ROM path
        Cartridge::load(this->path.c_str());
    }

    /**
        Perform a discrete "step" of the NES by rendering 1 frame.

        @param action the controller bitmap of which buttons to press.
        The parameter uses 1 for "pressed" and 0 for "not pressed".
        It uses the following mapping of bits to buttons:
        7: UP
        6: LEFT
        5: DOWN
        4: RIGHT
        3: SELECT
        2: START
        1: B
        0: A

    */
    void step(unsigned char action) {
        // run a frame on the CPU
        CPU::run_frame();
        // render the frame on the SDL surface
        GUI::render();
    }

    /**
        Return the screen of the emulator.

        @returns TODO

    */
    void get_screen() {

    }

};


// definitions of functions for the Python interface to access
extern "C" {
    /// The initializer to return a new NESEnv with a given path.
    NESEnv* NESEnv_init(wchar_t* path){ return new NESEnv(path); }

    /// The function to reset the environment.
    void NESEnv_reset(NESEnv* env) { env->reset(); }

    /// The function to perform a step on the emulator.
    void NESEnv_step(NESEnv* env, unsigned char action) { env->step(action); }

    /// The function to destroy an NESEnv and clear it from memory.
    void NESEnv_close(NESEnv* env) { delete env; }
}
