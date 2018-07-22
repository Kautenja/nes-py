#include "nes_env.hpp"
#include "cartridge.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "joypad.hpp"
#include "gui.hpp"

/**
    Initialize a new NESEnv.

    @param path the path to the ROM for the emulator to load
    @returns a new instance of NESEnv for a given ROM
*/
NESEnv::NESEnv(wchar_t* path) {
    // convert the wchar_t type to a string
    std::wstring ws_rom_path(path);
    std::string rom_path(ws_rom_path.begin(), ws_rom_path.end());
    // load the cartridge based on the environment's ROM path
    Cartridge::load(rom_path.c_str());
}

/// Reset the emulator to its initial state.
void NESEnv::reset() {
    CPU::power();
    PPU::reset();
}

/**
    Perform a discrete "step" of the NES by rendering 1 frame.

    @param action the controller bitmap of which buttons to press.
    The parameter uses 1 for "pressed" and 0 for "not pressed".
    It uses the following mapping of bits to buttons:
    7: RIGHT
    6: LEFT
    5: DOWN
    4: UP
    3: START
    2: SELECT
    1: B
    0: A
*/
void NESEnv::step(unsigned char action) {
    // write the action to the player's joy-pad
    Joypad::write_buttons(0, action);
    // run a frame on the CPU
    CPU::run_frame();
}

// definitions of functions for the Python interface to access
extern "C" {
    /// The initializer to return a new NESEnv with a given path.
    NESEnv* NESEnv_init(wchar_t* path){ return new NESEnv(path); }

    /// The width of the NES screen.
    unsigned NESEnv_width() { return GUI::get_width(); }

    /// The height of the NES screen.
    unsigned NESEnv_height() { return GUI::get_height(); }

    /// The getter for RAM access
    u8 NESEnv_read_mem(u16 address) { return CPU::read_mem(address); }

    /// The setter for RAM access
    void NESEnv_write_mem(u16 address, u8 value) { CPU::write_mem(address, value); }

    /// Copy the screen of the emulator to an output buffer (NumPy array)
    void NESEnv_screen(unsigned char *output_buffer) { GUI::copy_screen(output_buffer); }

    /// The function to reset the environment.
    void NESEnv_reset(NESEnv* env) { env->reset(); }

    /// The function to perform a step on the emulator.
    void NESEnv_step(NESEnv* env, unsigned char action) { env->step(action); }

    /// The function to destroy an NESEnv and clear it from memory.
    void NESEnv_close(NESEnv* env) { delete env; }
}
