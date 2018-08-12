#include "nes_env.hpp"

NESEnv::NESEnv(wchar_t* path) {
    // setup the game state
    current_state = new GameState();
    // convert the wchar_t type to a string
    std::wstring ws_rom_path(path);
    std::string rom_path(ws_rom_path.begin(), ws_rom_path.end());
    // initialize a cartridge and load the ROM for it
    current_state->cartridge = new Cartridge(rom_path.c_str());
    current_state->gui = new GUI();
    current_state->joypad = new Joypad();
    // set the cartridge pointer for the CPU and PPU
    current_state->load();

    backup_state = nullptr;
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

void NESEnv::backup() {
    // copy the current state with the backup state
    backup_state = new GameState(current_state);
    backup_state->cpu_state = CPU::get_state();
    backup_state->ppu_state = PPU::get_state();
}

void NESEnv::restore() {
    // copy the backup state into the current state and load the machine
    current_state = new GameState(backup_state);
    current_state->load();
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
    u8 NESEnv_read_mem(NESEnv* env, u16 address) {
        return CPU::read_mem(address);
    }

    /// The setter for RAM access
    void NESEnv_write_mem(NESEnv* env, u16 address, u8 value) {
        CPU::write_mem(address, value);
    }

    /// Copy the screen of the emulator to an output buffer (NumPy array)
    void NESEnv_screen(NESEnv* env, unsigned char *output_buffer) {
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

    /// The function to backup the game-state
    void NESEnv_backup(NESEnv* env) {
        env->backup();
    }

    /// The function to restore the game-state
    void NESEnv_restore(NESEnv* env) {
        env->restore();
    }

}
