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
    // set the backup state to NULL
    backup_state = nullptr;
}

void NESEnv::reset() {
    // initialize the CPU
    CPU::power();
    // initialize the PPU
    PPU::reset();
}

void NESEnv::step(unsigned char action) {
    // write the action to the player's joy-pad
    CPU::get_joypad()->write_buttons(0, action);
    // run a frame on the CPU
    CPU::run_frame();
}

void NESEnv::backup() {
    // delete any current backup
    delete backup_state;
    // copy the current state as the backup state
    backup_state = new GameState(current_state, CPU::get_state(), PPU::get_state());
}

void NESEnv::restore() {
    // delete the current state in progress
    delete current_state;
    // copy the backup state into the current state
    current_state = new GameState(backup_state);
    // load the current state into the machine
    current_state->load();
}
