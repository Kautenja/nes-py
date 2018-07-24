#include "nes_env.hpp"
#include "cartridge.hpp"
#include "cpu.hpp"
#include "ppu.hpp"
#include "gui.hpp"
#include "joypad.hpp"

NESEnv::NESEnv(wchar_t* path) {
    // convert the wchar_t type to a string
    std::wstring ws_rom_path(path);
    std::string rom_path(ws_rom_path.begin(), ws_rom_path.end());
    // initialize a cartridge and load the ROM for it
    Cartridge* cartridge = new Cartridge(rom_path.c_str());

    // set the cartridge pointer for the CPU and PPU
    CPU::set_cartridge(cartridge);
    PPU::set_cartridge(cartridge);

    // setup the joy-pad for this environment's CPU
    CPU::set_joypad(new Joypad());
    // setup the GUI for this environment's PPU
    PPU::set_gui(new GUI());
}

void NESEnv::reset() {
    CPU::power();
    PPU::reset();
}

void NESEnv::step(unsigned char action) {
    // write the action to the player's joy-pad
    CPU::get_joypad()->write_buttons(0, action);
    // run a frame on the CPU
    CPU::run_frame();
}

// definitions of functions for the Python interface to access
extern "C" {
    /// The initializer to return a new NESEnv with a given path.
    NESEnv* NESEnv_init(wchar_t* path){
        return new NESEnv(path);
    }

    /// The width of the NES screen.
    unsigned NESEnv_width() {
        return GUI::get_width();
    }

    /// The height of the NES screen.
    unsigned NESEnv_height() {
        return GUI::get_height();
    }

    /// The getter for RAM access
    u8 NESEnv_read_mem(u16 address) {
        return CPU::read_mem(address);
    }

    /// The setter for RAM access
    void NESEnv_write_mem(u16 address, u8 value) {
        CPU::write_mem(address, value);
    }

    /// Copy the screen of the emulator to an output buffer (NumPy array)
    void NESEnv_screen(unsigned char *output_buffer) {
        PPU::get_gui()->copy_screen(output_buffer);
    }

    /// The function to reset the environment.
    void NESEnv_reset(NESEnv* env) {
        env->reset();
    }

    /// The function to perform a step on the emulator.
    void NESEnv_step(NESEnv* env, unsigned char action) {
        env->step(action);
    }

    /// The function to destroy an NESEnv and clear it from memory.
    void NESEnv_close(NESEnv* env) {
        delete env;
    }
}
