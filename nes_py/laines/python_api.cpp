/// File: python_api.py
/// Description: The API definition for ctypes in Python.
///
#include "nes_env.hpp"

// Windows-base systems
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
    // setup the module initializer. required to link visual studio C++ ctypes
    void PyInit_lib_nes_env() { }
    // setup the function modifier to export in the DLL
    #define exp __declspec(dllexport)
// Unix-based systems
#else
    // setup the modifier as a dummy
    #define exp
#endif

// definitions of functions for the Python interface to access
extern "C" {
    /// The initializer to return a new NESEnv with a given path.
    exp NESEnv* NESEnv_init(wchar_t* path){
        return new NESEnv(path);
    }

    /// The width of the NES screen.
    exp unsigned NESEnv_width() {
        return GUI::get_width();
    }

    /// The height of the NES screen.
    exp unsigned NESEnv_height() {
        return GUI::get_height();
    }

    /// The getter for RAM access
    exp u8 NESEnv_read_mem(NESEnv* env, u16 address) {
        return CPU::read_mem(address);
    }

    /// The setter for RAM access
    exp void NESEnv_write_mem(NESEnv* env, u16 address, u8 value) {
        CPU::write_mem(address, value);
    }

    /// Copy the screen of the emulator to an output buffer (NumPy array)
    exp void NESEnv_screen(NESEnv* env, unsigned char *output_buffer) {
        PPU::get_gui()->copy_screen(output_buffer);
    }

    /// The function to reset the environment.
    exp void NESEnv_reset(NESEnv* env) {
        env->reset();
    }

    /// The function to perform a step on the emulator.
    exp void NESEnv_step(NESEnv* env, unsigned char action) {
        env->step(action);
    }

    /// The function to destroy an NESEnv and clear it from memory.
    exp void NESEnv_close(NESEnv* env) {
        delete env;
    }

    /// The function to backup the game-state
    exp void NESEnv_backup(NESEnv* env) {
        env->backup();
    }

    /// The function to restore the game-state
    exp void NESEnv_restore(NESEnv* env) {
        env->restore();
    }

}
